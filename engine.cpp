#include "io.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "persist_weights.h"
#include "imu_features.h"

FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

void runIteration() {
	SerialCommandType order = Coms.receive();
	switch (order) {
		case CMD_NONE:
			return;
		case CMD_COLLECT: {
			Coms.send("[CMD=COLLECT]: INIT");
			// This will set nSamples
			collectWindow(windowBuffer, &nSamples);
			Coms.send("[CMD=COLLECT]: COLLECTED");
			// This will set equally many labels
			if (!Coms.getLabel(labelsBuffer, nSamples)) {
				Coms.send("Bad input");
				nSamples = 0;
			}
			break;
		}
		case CMD_TRAIN: {
			Coms.send("[CMD=TRAIN]: INIT");
			Coms.send("[CMD=TRAIN]: UPDATE EMA");
			updateEMA(windowBuffer, nSamples);
			Coms.send("[CMD=TRAIN]: NORMALIZATION");
			normalizeWindow(windowBuffer, nSamples);
			Coms.send("[CMD=TRAIN]: GRADIENT DESCENT");
			trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
			size_t i = 0;
			Coms.send("[CMD=TRAIN]: PRINTING PREDICTIONS:");
			while (nSamples--) {
				updateReservoir(windowBuffer[i++]);
				uint8_t prediction = predict();
				Coms.send(String(prediction));
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
				Coms.send(String(prediction));
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
				Coms.send(String(prediction));
			}
			nSamples = 0;
			break;
		}
		case CMD_STOP:
			break;
		// TODO: case CMD_SHARE_WEIGHTS:
		// TODO: case PERSIST_WEIGHTS:
	}
}
