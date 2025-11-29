#include "engine.h"
#include "imu_features.h"
#include "serial_protocol.h"
#include "io.h"

static char lineBuffer[MAX_LINE_LENGTH];
static size_t linePos = 0;

void initSerial() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }
}

// Read a line from Serial into 'out', return true if a complete line was read
bool readRawSerialLine(String& out) {
    while (Serial.available()) {
        char c = Serial.read();
	
        if (c == '\n' || c == '\r') {
            if (linePos > 0) {
                lineBuffer[linePos] = '\0';
                out = String(lineBuffer);
                linePos = 0;
                return true;
            }
        } else {
            if (linePos < MAX_LINE_LENGTH - 1) {
                lineBuffer[linePos++] = c;
            }
        }
    }
    return false;
}

// Parse a command line into a SerialCommandType
SerialCommandType parseSerialCommand(String& line) {
    line.trim();

    if (line == "TRAIN") {
        return CMD_TRAIN;
    } else if (line == "COLLECT") {
        return CMD_COLLECT;
    } else if (line == "VALIDATE") {
        return CMD_VAL;
    } else if (line == "INFER") {
        return CMD_INFER;
    } else if (line == "STOP") {
        return CMD_STOP;
    }
    return CMD_NONE;
}

// Read a line from Serial and parse it into a command
// SerialCommandType readSerialCommand() {
//     SerialCommandType cmd;
//     cmd = CMD_NONE;

//     while (Serial.available()) {
//         char c = Serial.read();
	
//         if (c == '\n' || c == '\r') {
//             if (linePos > 0) {
//                 lineBuffer[linePos] = '\0';
//                 String line = String(lineBuffer);
//                 linePos = 0;

//                 line.trim();

//                 // Parse commands
//                 if (line == "TRAIN") {
//                     cmd = CMD_TRAIN;
//                 } else if (line == "COLLECT") {
// 		    cmd = CMD_COLLECT;
//                 } else if (line == "VALIDATE") {
// 		    cmd = CMD_VAL;
//                 } else if (line == "INFER") {
//                     cmd = CMD_INFER;
//                 } else if (line == "STOP") {
//                     cmd = CMD_STOP;
//                 }
//                 return cmd;
//             }
//         } else {
//             if (linePos < MAX_LINE_LENGTH - 1) {
//                 lineBuffer[linePos++] = c;
//             }
//         }
//     }

//     return cmd; // No complete command yet
// }

// bool getLabel(uint8_t (&labelBuffer)[WINDOW_SIZE], uint16_t nSamples) {
// 	char dataBuffer[2];
// 	Serial.print("Label: ");
	
// 	// Wait for label OR timeout (5s)
// 	unsigned long start = millis();
// 	while (Serial.available() == 0 && (millis() - start) < 5000) {
//     		;
//   	}
// 	int bytesRead = Serial.readBytesUntil('\n', dataBuffer, 1);
// 	if (bytesRead == 0) {
// 		// No good data
//         	return false;
//     	}
// 	dataBuffer[bytesRead] = '\0';
// 	uint8_t label = (uint8_t) atol(dataBuffer);
// 	for (size_t i = 0; i < nSamples; i++) {
// 		labelBuffer[i] = label;
// 	}
// 	return true;
// }

// // Send generic message
// void sendMessage(const String& msg) {
//     Serial.println(msg);
// }

