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
			// Return early no message or action (default)
			return;
		case CMD_STOP:
			nSamples = 0;
			mode = IDLE;
			sendMessage("[CMD=STOP]: RECEIVED");
			// Defer done message
			msg = "[CMD=STOP]: DONE";
			break;
		case CMD_COLLECT:
			mode = COLLECTION;
			sendMessage("[CMD=COLLECT]: RECEIVED");
			// Defer done message
			msg = "[CMD=COLLECT]: DONE";
			collectWindow(windowBuffer);
			transmitCollectedWindow(windowBuffer);
			break;
		case CMD_TRAIN:
			mode = TRAINING;
			sendMessage("[CMD=TRAIN]: RECEIVED") ;
			// Defer done message
			msg = "[CMD=TRAIN]: DONE";
			sendMessage("[IO]: STREAM");
			parseDataStream(windowBuffer, &nSamples, labelsBuffer);
			if (nSamples == 0) {
				sendMessage("[IO]: TIMEOUT OR MALFORMED");
				long currentTimeout = Serial.getTimeout();
				Serial.print("Current Serial Timeout (ms): ");
				Serial.println(currentTimeout);
			} else {
				sendMessage("[TRAIN]: START");
            			trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
				Serial.println("<result>");
				size_t i = 0;
				while (nSamples--) {
					updateReservoir(windowBuffer[i++]);
					uint8_t prediction = predict();
					sendPrediction(prediction, mode);
				}
				Serial.println("</result>");
				sendMessage("[TRAIN]: DONE");
			}
		     	break;
		case CMD_VAL:
		     	mode = VALIDATION;
			sendMessage("[CMD=VAL]: RECEIVED");
			// Defer done message
			msg = "[CMD=VAL]: DONE";
			parseDataStream(windowBuffer, &nSamples, labelsBuffer);
			if (nSamples == 0) {
				sendMessage("[IO]: TIMEOUT OR MALFORMED");
				long currentTimeout = Serial.getTimeout();
				Serial.print("[INFO]: Current Serial Timeout (ms): ");
				Serial.println(currentTimeout);
			} else {
				sendMessage("[VALIDATION]: START");
				Serial.println("<result>");
				size_t i = 0;
				while (nSamples--) {
					updateReservoir(windowBuffer[i++]);
					uint8_t prediction = predict();
					sendPrediction(prediction, mode);
				}
				Serial.println("</result>");
				sendMessage("[VALIDATION]: DONE");
			}
			break;
	}
	sendMessage(msg);
}
