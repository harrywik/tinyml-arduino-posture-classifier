#include <ArduinoBLE.h>
#include "io.h"
#include "ble.h"
#include "serial_protocol.h"
#include "imu_features.h" 
#include "button.h"

extern SerialCommandType parseSerialCommand(String& line);
extern bool readRawSerialLine(String& out);

extern void communicateUSBMode(void);
extern void communicateBLEMode(void);

IO Coms;

void IO::setBackend(CommunicationMode mode) {
if (mode == MODE_BLE) {
        currentBackend = IO_BLE;
    	initBLE();   
        communicateBLEMode(); // blue LED
    } else { // USB
        currentBackend = IO_SERIAL;
    	initSerial(); 
        communicateUSBMode(); // red LED
    }
}

void IO::send(const String& msg){
    if(currentBackend == IO_SERIAL){
        Serial.println(msg);
    } else {
        BLESend(msg); 
    }
}

SerialCommandType IO::receive() {
    String rawCommand;
    if (currentBackend == IO_SERIAL) {
        if (readRawSerialLine(rawCommand)) {
            return parseSerialCommand(rawCommand);
        }
    } else {
        if (BLEReceive(rawCommand)) {
            // BLE mode received a command
            return parseSerialCommand(rawCommand);
        } 
    }
    return CMD_NONE;
}

bool IO::getLabel(uint8_t* labelBuffer, uint16_t nSamples) {
    if (currentBackend == IO_SERIAL) {
        char dataBuffer[2];
        while (Serial.available()) { 
            Serial.read(); 
        }
        Serial.print("Label: ");
        
        unsigned long start = millis();
        while (Serial.available() == 0 && (millis() - start) < 8000) {
            ;
        }
       
        int bytesRead = Serial.readBytesUntil('\n', dataBuffer, sizeof(dataBuffer) - 1); 
        
        if (bytesRead == 0) {
            return false;
        }
        dataBuffer[bytesRead] = '\0';
        
        while (Serial.available()) {
            Serial.read();
        }
        uint8_t label = (uint8_t) atol(dataBuffer); 
        
	// Range check
        if (label >= NUM_FEATURES)
            return false;
        for (size_t i = 0; i < nSamples; i++) {
            labelBuffer[i] = label;
        }
        return true;
    } else if (currentBackend == IO_BLE) {
        // send request for label
        BLESend("Label: "); 
        
        String labelCommand;
        unsigned long start = millis();
        const unsigned long timeoutMs = 5000; // timeout for label input
	bool received = false;

        // wait for label command or timeout
        while ((millis() - start) < timeoutMs && !received) {
		if (BLEReceive(labelCommand))
			received = true;
            	delay(10); 
        }
	if (received) {
		uint8_t label = (uint8_t) atol(labelCommand.c_str()); 
		// Range check
		if (label >= NUM_FEATURES)
			return false;
		for (size_t i = 0; i < nSamples; i++) {
		    labelBuffer[i] = label;
		}
		return true;
	}

        // timeout
        BLESend("[ERROR]: LABEL TIMEOUT\n"); 
        return false;
    }
    
    return false;
}

bool IO::getUUID(void) {
    if (currentBackend == IO_SERIAL) {
        uuid address;

        Serial.print("My UUID (BLE_SLAVE_UUID): ");
        Serial.println(BLE_SLAVE_UUID);

        while (Serial.available()) { 
            Serial.read(); 
        }
        Serial.print("Counterparty UUID (or Empty for peripheral): ");

        unsigned long start = millis();
        while (Serial.available() == 0 && (millis() - start) < 30000) {
            delay(5);
            BLE.poll();
        }

        // ---- Case 1: timeout -> PERIPHERAL ----
        if (Serial.available() == 0) {
            Serial.println("\nNo input -> peripheral mode");
            currentBLEMode = WS_BLE_PERIPHERAL;

            // IMPORTANT: start advertising immediately
            if (!initBLE()) {
                Serial.println("initBLE() failed");
            } else {
                Serial.println("Peripheral: initBLE() OK, advertising started");
            }

            return false;
        }

        // read until "\n"
        int bytesRead = Serial.readBytesUntil('\n', address, sizeof(address) - 1);
        address[bytesRead] = '\0';

        // remove trailing '\r' if present
        if (bytesRead > 0 && address[bytesRead - 1] == '\r') {
            address[bytesRead - 1] = '\0';
            bytesRead--;
        }

        while (Serial.available()) {
            Serial.read();
        }

        // ---- Case 2: empty line -> PERIPHERAL ----
        if (bytesRead == 0) {
            Serial.println("\nEmpty input -> peripheral mode");
            currentBLEMode = WS_BLE_PERIPHERAL;

            if (!initBLE()) {
                Serial.println("initBLE() failed");
            } else {
                Serial.println("Peripheral: initBLE() OK, advertising started");
            }

            return false;
        }

        // ---- Case 3: non-empty -> CENTRAL ----
        // (optional) trim leading/trailing spaces just in case copy/paste adds them
        // simple trim:
        while (bytesRead > 0 && address[0] == ' ') { memmove(address, address + 1, --bytesRead); address[bytesRead] = '\0'; }
        while (bytesRead > 0 && address[bytesRead - 1] == ' ') { address[--bytesRead] = '\0'; }

        Serial.print("\nCentral will scan UUID: [");
        Serial.print(address);
        Serial.println("]");

        strcpy(peripheral, address);
        Serial.println("Peripheral set to: ");
        Serial.print(peripheral);
        currentBLEMode = WS_BLE_CENTRAL;

        return true;
    }

    // TODO: IO_BLE backend
    return false;
}

bool IO::sendModel(float* weights, size_t len) {
    if (currentBackend != IO_SERIAL) {
        Serial.println("IO::sendModel() only supported in SERIAL mode");
        return false;
    }

    if (currentBLEMode == WS_BLE_CENTRAL) {

        Serial.println("IO::sendModel() as CENTRAL");
        if (!startCentralService(peripheral)) {
            Serial.println("startCentralService() failed");
            return false;
        }

        // startCentralService(peripheral);
        unsigned long start = millis();

        // wait for succesfull connection
        while (!attemptConnectionToPeripheral(peripheral) && (millis() - start) < 30000) {
            BLE.poll();
            delay(10);
        }
        if (millis() - start >= 30000) {
            // Timeout broke the loop
            Serial.println("Timeout while waiting for peripheral");
            return false;
        }
    }

    if (currentBLEMode == WS_BLE_PERIPHERAL && !isBLEConnected()) {
        Serial.println("IO::sendModel() as PERIPHERAL");
        initBLE();
        unsigned long start = millis();
        while (!isBLEConnected() && (millis() - start) < 35000) {
            BLE.poll();
            delay(10);
            readvertiseBLE();
        }
        if (!isBLEConnected()) {
            Serial.println("Timeout while waiting for central");
            return false;
        }
    }

		// Peripheral logic
    Serial.println("sendModel()");
    return weightShareSend(currentBLEMode, MSG_TYPE_WEIGHTS, (uint8_t*) weights, len);

	// TODO:
	// fix for bluetooth computer connection as well...

}

bool IO::sendNBatches(const uint16_t n_a, size_t len) {
	if (currentBackend == IO_SERIAL) {
		Serial.println("SendNBatches()");
		bool res =  weightShareSend(currentBLEMode, MSG_TYPE_BATCH_COUNT, (uint8_t*) &n_a, len);
		if (currentBLEMode == WS_BLE_PERIPHERAL)
			// Return to previous state
			deinitAsPeripheral();
		return res;
	}
}

bool IO::receiveModel(float* weights, size_t len) {
	if (currentBackend == IO_SERIAL) {
		if (currentBLEMode == WS_BLE_PERIPHERAL && !isBLEConnected()) {
			// Extra logic block as peripheral - only init if not connected
			initBLE();

			while(!isBLEConnected()) {
				BLE.poll();
				delay(10);
				readvertiseBLE();
			}
		}
		Serial.println("receiveModel()");
		// receive
		return weightShareReceive(currentBLEMode, MSG_TYPE_WEIGHTS, (uint8_t*) weights, len);
	}
	// TODO:
	// fix for bluetooth computer connection as well...
	return false;
}

bool IO::receiveNBatches(uint16_t *n_b, size_t len) {
	if (currentBackend == IO_SERIAL) {
		Serial.println("receiveNBatches()");
		return weightShareReceive(currentBLEMode, MSG_TYPE_BATCH_COUNT, (uint8_t*) n_b, len);
	}
	// TODO:
	// fix for bluetooth computer connection as well...
	return false;
}

