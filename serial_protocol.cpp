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
    } else if (line == "VALIDATE") {
        return CMD_VAL;
    } else if (line == "VALIDATE_DONE") {
        return CMD_VAL_DONE;
    } else if (line == "INFER") {
        return CMD_INFER;
    } else if (line == "RESET") {
        return CMD_RESET;
    } else if (line == "PERSIST") {
        return CMD_PERSIST;
    } else if (line == "SHARE") {
        return CMD_SHARE_WEIGHTS;
    }
    return CMD_NONE;
}

