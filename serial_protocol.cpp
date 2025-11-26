#include "imu_features.h"
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
SerialCommandType readSerialCommand() {
    SerialCommandType cmd;
    cmd = CMD_NONE;

    while (Serial.available()) {
        char c = Serial.read();
	
        if (c == '\n' || c == '\r') {
            if (linePos > 0) {
                lineBuffer[linePos] = '\0';
                String line = String(lineBuffer);
                linePos = 0;

                line.trim();

                // Parse commands
                if (line == "TRAIN") {
                    cmd = CMD_TRAIN;
                } else if (line == "VALIDATE") {
		    cmd = CMD_VAL;
                } else if (line == "INFER") {
                    cmd = CMD_INFER;
                } else if (line == "STOP") {
                    cmd = CMD_STOP;
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


// --- Data Reading Function ---
//
// wraps parseWindow()
//

bool parseDataStream(FeatureVector (&windowBuffer)[WINDOW_SIZE], uint16_t* nSamples, uint8_t (&labelsBuffer)[WINDOW_SIZE]) {

    /*
     *
     * Sets the following variables:
     *
     * FeatureVector windowBuffer[WINDOW_SIZE];
     * uint8_t labelsBuffer[WINDOW_SIZE]; 
     * uint16_t nSamples; 
     *
     */

    const size_t BUFFER_SIZE = (NUM_FEATURES * MAX_FLOAT_LEN) + MAX_UINT8_LEN + NUM_DELIMITERS + NULL_TERM_LEN;
    char dataBuffer[BUFFER_SIZE];

    *nSamples = 0;

    while (*nSamples < WINDOW_SIZE && parseWindow(windowBuffer[*nSamples], &labelsBuffer[*nSamples], dataBuffer, BUFFER_SIZE))
	(*nSamples)++;
    
    return *nSamples == WINDOW_SIZE;
}

// --- main parser of data stream ---
// Expected input feature,...,feature,label
// Max decimals in feature is #MAX_FLOAT_LEN
//
// return success status
//
bool parseWindow(FeatureVector& featureWindow, uint8_t* label, char* dataBuffer, size_t bufferSize) {
    // Read the entire incoming line until a newline ('\n')
    int bytesRead = Serial.readBytesUntil('\n', dataBuffer, bufferSize - 1);

    if (bytesRead == 0) {
        return false;
    }
    dataBuffer[bytesRead] = '\0'; // Null-terminate the string

    // Tokenize the string using ',' as the delimiter
    char* token;
    token = strtok(dataBuffer, ",");

    // Loop through and parse the expected number of float features
    for (size_t i = 0; i < NUM_FEATURES; i++) {
        if (token == NULL) {
            // Did not find enough tokens
            Serial.println("Error: Insufficient feature tokens.");
            return false;
        }

        // Convert the token string to a float
        featureWindow.features[i] = atof(token);

        // Get the next token (will return NULL if no more tokens are found)
        token = strtok(NULL, ","); 
    }

    // Parse the final uint8_t label
    if (token == NULL) {
        Serial.println("Error: Missing label token.");
        return false;
    }

    // Convert the token string to an integer
    long tempLabel = atol(token); // Temporarily a long

    // Validate and assign the label
    //
    // Range check...
    if (tempLabel >= 0 && tempLabel <= 255) {
	// Cast to uint8_t and set value
        *label = (uint8_t)tempLabel; 
        return true;
    } else {
        Serial.print("Label out of range: ");
        Serial.println(tempLabel);
        return false;
    }
}

// Send training loss
void sendTrainLoss(float loss) {
    Serial.print("TRAIN_LOSS:");
    Serial.println(loss, 4);
}

// Send validation loss
void sendValLoss(float loss) {
    Serial.print("VAL_LOSS:");
    Serial.println(loss, 4);
}

// Send prediction result
void sendPrediction(uint8_t label) {
    Serial.print("PRED:");
    Serial.println(label);
}

// Send generic message
void sendMessage(const String& msg) {
    Serial.println(msg);
}

