#include "main.h"
#include "imu_features.h"
#include "esn.h"
#include "serial_protocol.h"

OperationMode mode = IDLE;
FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

void actOnCommand(SerialCommand order) {
	String msg;
	switch (order.type) {
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
			collect();
			transmitCollected();
			break;
		case CMD_TRAIN:
			mode = TRAINING;
			msg = "Parsed incoming data and trained.";
			parseDataStream();
			sendMessage("[Training]: START")
            		trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
			sendMessage("[Training]: DONE")
		     	break;
		case CMD_VAL:
		     	mode = TRAINING;
			msg = "Parsed incoming data and predicted.";
			parseDataStream();
			predict();
			break;
	}
	sendMessage(msg);
}

void main(void) {

	SerialCommand cmd = readSerialCommand();
	actOnCommand(cmd);
















    updateIMU();

    if (collecting && training) {
        sendMessage("Cannot collect and train simultaneuosly");
        return;
    }

    if (collecting) {
        FeatureVector fv = computeFeatures();
        sendFeatureVector(fv);
    }

    if (training) {
      
    }

    switch (cmd.type) {
        case CMD_COLLECT:
            collecting = true;
            sendMessage("Feature collection started");
            break;
        case CMD_LABEL:
            collecting = false;
            // Save labeled feature for training
            if (nSamples < WINDOW_SIZE) {
                windowBuffer[nSamples] = fv;
                labelsBuffer[nSamples] = cmd.label;
                nSamples++;
                sendMessage("Sample stored");
            } else {
                sendMessage("Buffer full!");
            }

            break;

        case CMD_TRAIN:
            collecting = false;
            sendMessage("Training completed");
            nSamples = 0;
            break;

        case CMD_INFER: {
            collecting = false;
            uint8_t pred = predict();
            sendPrediction(pred);
            break;
        }

        case CMD_STOP:
            collecting = false;
            sendMessage("Stopping loop");
            break;
        case CMD_NONE:
            break;
        default:
            break;
    }
}
