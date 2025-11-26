#include "engine.h"
#include "imu_features.h"
#include "esn.h"
#include "serial_protocol.h"

OperationMode mode = IDLE;
FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

void runIteration(void) {
	SerialCommandType order = readSerialCommand();
	String msg;
	switch (order) {
		case CMD_NONE:
			return;
		case CMD_STOP:
			nSamples = 0;
			mode = IDLE;
			msg = "Stop command received";
			break;
		case CMD_COLLECT:
			mode = COLLECTION;
			msg = "Collection initiated";
			// collect();
			// transmitCollected();
			break;
		case CMD_TRAIN:
			mode = TRAINING;
			msg = "Parsed incoming data and trained.";
			parseDataStream(windowBuffer, &nSamples, labelsBuffer);
			sendMessage("[Training]: START");
            		trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
			sendMessage("[Training]: DONE");
		     	break;
		case CMD_VAL:
		     	mode = TRAINING;
			msg = "Parsed incoming data and predicted.";
			parseDataStream(windowBuffer, &nSamples, labelsBuffer);
			// predict();
			break;
	}
	sendMessage(msg);
}

