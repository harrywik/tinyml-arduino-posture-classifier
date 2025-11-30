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
        Serial.print("Label: ");
        
        unsigned long start = millis();
        while (Serial.available() == 0 && (millis() - start) < 5000) {
            ;
        }
       
        int bytesRead = Serial.readBytesUntil('\n', dataBuffer, 1); 
        
        if (bytesRead == 0) {
            return false;
        }
        dataBuffer[bytesRead] = '\0';
        uint8_t label = (uint8_t) atol(dataBuffer); 
        
        for (size_t i = 0; i < nSamples; i++) {
            labelBuffer[i] = label;
        }
        return true;
    } else if (currentBackend == IO_BLE) {
        // send request for label
        BLESend("[REQUEST]: Label"); 
        
        String labelCommand;
        unsigned long start = millis();
        const unsigned long timeoutMs = 5000; // timeout for label input

        // wait for label command or timeout
        while ((millis() - start) < timeoutMs) {
            if (BLEReceive(labelCommand)) {
                // check if it's a label command
                if (labelCommand.startsWith("Label:")) {
                    // parse label value
                    String valueStr = labelCommand.substring(6); 
                    uint8_t label = (uint8_t) valueStr.toInt();

                    // fill label buffer
                    for (uint16_t i = 0; i < nSamples; i++) {
                        labelBuffer[i] = label;
                    }
                    BLESend("[Label]: RECEIVED");
                    return true;
                }
            }
            delay(10); 
        }

        // timeout
        send("[ERROR]: LABEL TIMEOUT");
        return false;
    }
    
    return false;
}

// ONLY BLE supported for model exchange
bool IO::sendModel(float* weights, size_t len) {
    if (currentBackend == IO_BLE) {
        return BLESendModel(weights, len);
    }
    return false;
}

bool IO::receiveModel(float* weights, size_t len) {
    if (currentBackend == IO_BLE) {
        return BLEReceiveModel(weights, len);
    }
    return false;
}
