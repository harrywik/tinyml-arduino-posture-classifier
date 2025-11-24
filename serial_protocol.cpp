#include "serial_protocol.h"

static char lineBuffer[MAX_LINE_LENGTH];
static size_t linePos = 0;

void initSerial() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }
}

// Read a line from Serial and parse it into a command
SerialCommand readSerialCommand() {
    SerialCommand cmd;
    cmd.type = CMD_NONE;

    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (linePos > 0) {
                lineBuffer[linePos] = '\0';
                String line = String(lineBuffer);
                linePos = 0;

                line.trim();
                line.toUpperCase();

                // Parse commands
                if (line.startsWith("LABEL:")) {
                    cmd.type = CMD_LABEL;
                    cmd.argument = parseLabel(line.substring(6)); // after "LABEL:"
                } else if (line == "TRAIN") {
                    cmd.type = CMD_TRAIN;
                } else if (line == "INFER") {
                    cmd.type = CMD_INFER;
                } else if (line == "STOP") {
                    cmd.type = CMD_STOP;
                }
                return cmd;
            }
        } else {
            if (linePos < MAX_LINE_LENGTH - 1) {
                lineBuffer[linePos++] = c;
            }
        }
    }

    return cmd; // No complete command yet
}

// Send training loss
void sendTrainLoss(float loss) {
    Serial.print("TRAIN_LOSS:");
    Serial.println(loss, 4);
}

// Parse the incoming string
uint8_t parseLabel(const String& str) {
    switch(str) {
        case "SITTING": 
        case "0":
                return 0;
        case "1":
        case "STANDING": 
                return 1;
        case "2":
        case "SITTING": 
                return 2;
        case "3":
        case "ACTIVE": 
                return 3;
	default:
    		return 255; // Invalid
    }
}

// Send validation loss
void sendValLoss(float loss) {
    Serial.print("VAL_LOSS:");
    Serial.println(loss, 4);
}

// Send prediction result
void sendPrediction(unint8_t label) {
    Serial.print("PRED:");
    Serial.println(label);
}

// Send generic message
void sendMessage(const String& msg) {
    Serial.println(msg);
}

