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
if (mode == BLE) {
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

        while (Serial.available()) { 
            Serial.read(); 
        }
        Serial.print("Counterparty UUID: ");

        unsigned long start = millis();
        while (Serial.available() == 0 && (millis() - start) < 30000) {
            ;
        }

        int bytesRead = Serial.readBytesUntil('\n', address, sizeof(address) - 1); 

        if (bytesRead != sizeof(address) - 1) {
	    currentBLEMode = WS_BLE_PERIPHERAL;
            return false;
        }

        address[bytesRead] = '\0';

        while (Serial.available()) {
            Serial.read();
        }
	peripheral = address;
	currentBLEMode = WS_BLE_CENTRAL;
        return true;
    } else if (currentBackend == IO_BLE) {
	//TODO:
	// implement this for bluetooth as well
    }

    return false;
}

bool IO::sendModel(float* weights, size_t len) {
	if (currentBackend == IO_SERIAL) {
		if (currentBLEMode == WS_BLE_CENTRAL) {
			startCentralService(peripheral);
			unsigned long start = millis();
			// wait for succesfull connection
			while (!attemptConnectionToPeripheral() && (millis() - start) < 35000) {
			    ;
			}
			if (millis() - start >= 35000)
				// Timeout broke the loop
				return false;

			// Central logic
			return weightShareSend(currentBLEMode, MSG_TYPE_WEIGHTS, (const uint8_t*) weights, len);
		}
		// Peripheral logic
		return weightShareSend(currentBLEMode, MSG_TYPE_WEIGHTS, (const uint8_t*) weights, len);
	}
	// TODO:
	// fix for bluetooth computer connection as well...
	return false;
}

bool IO::sendNBatches(const uint16_t n_a, size_t len) {
	if (currentBackend == IO_SERIAL) {
		bool res =  weightShareReceive(currentBLEMode, MSG_TYPE_BATCH_COUNT, (const uint8_t*) &n_a, len);
		if (currentBLEMode == WS_BLE_PERIPHERAL)
			// Return to previous state
			deinitAsPeripheral();
		return res;
	}
}

bool IO::receiveModel(float* weights, size_t len) {
	if (currentBackend == IO_SERIAL) {
		if (currentBLEMode == WS_BLE_PERIPHERAL) {
			// Extra logic block as peripheral
			initBLE();

			while(!isBLEConnected()) {
				delay(500);
				readvertiseBLE();
			}
		}
		// receive
		return weightShareReceive(currentBLEMode, MSG_TYPE_WEIGHTS, (uint8_t*) weights, len);
	}
	// TODO:
	// fix for bluetooth computer connection as well...
	return false;
}

bool IO::receiveNBatches(uint16_t *n_b, size_t len) {
	if (currentBackend == IO_SERIAL) {
		return weightShareReceive(currentBLEMode, MSG_TYPE_BATCH_COUNT, (uint8_t*) n_b, len);
	}
	// TODO:
	// fix for bluetooth computer connection as well...
	return false;
}

