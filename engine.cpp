#include "engine.h"
#include "imu_features.h"
#include "esn.h"
#include "serial_protocol.h"

OperationMode mode = IDLE;
FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

void runIteration(void) {

	SerialCommandType cmd = readSerialCommand();
	switch (cmd) {
		case CMD_NONE:
			return;
		case CMD_COLLECT: {
			Serial.println("[CMD=COLLECT]: INIT");
			// This will set nSamples
			collectWindow(windowBuffer, &nSamples);
			Serial.println("[CMD=COLLECT]: COLLECTED");
			// This will set equally many labels
			if (!getLabel(labelsBuffer, nSamples)) {
				Serial.println("Bad input");
				nSamples = 0;
			}
			break;
		}
		case CMD_TRAIN: {
			Serial.println("[CMD=TRAIN]: INIT");
			Serial.println("[CMD=TRAIN]: UPDATE EMA");
			updateEMA(windowBuffer, nSamples);
			Serial.println("[CMD=TRAIN]: NORMALIZATION");
			normalizeWindow(windowBuffer, nSamples);
			Serial.println("[CMD=TRAIN]: GRADIENT DESCENT");
			trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
			size_t i = 0;
			Serial.println("[CMD=TRAIN]: PRINTING PREDICTIONS:");
			while (nSamples--) {
				updateReservoir(windowBuffer[i++]);
				uint8_t prediction = predict();
				Serial.println(prediction);
			}
			nSamples = 0;
			break;
		}
		case CMD_VAL: {
			normalizeWindow(windowBuffer, nSamples);
			size_t i = 0;
			while (nSamples--) {
				updateReservoir(windowBuffer[i++]);
				uint8_t prediction = predict();
				Serial.println(prediction);
			}
			nSamples = 0;
			break;
		}
		case CMD_INFER: {
			collectWindow(windowBuffer, &nSamples);
			normalizeWindow(windowBuffer, nSamples);
			size_t i = 0;
			while (nSamples--) {
				updateReservoir(windowBuffer[i++]);
				uint8_t prediction = predict();
				Serial.println(prediction);
			}
			nSamples = 0;
			break;
		}
		case CMD_STOP:
			break;
	}
}
