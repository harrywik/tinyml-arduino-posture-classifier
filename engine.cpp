#include "io.h"
#include "led.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "persistance.h"
#include "imu_features.h"
#include "eval.h"

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
			trainOutputLayer(windowBuffer, labelsBuffer, nSamples, LEARNING_RATE);			
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
			// normalizeWindow(windowBuffer, nSamples);
			// size_t i = 0;
			//TODO:
			// require trainWindow, validateWindow, trainLabel, validateLabel to be defined elsewhere
			//
			evaluateLoop(true);
			// while (nSamples--) {
			// 	// updateReservoir(windowBuffer[i++]);
			// 	// uint8_t prediction = predict();
			// 	// Coms.send(String(prediction));
				
			// }
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
		case CMD_RESET: {
			Coms.send("[CMD=RESET]: INIT");
			if (rmKVpersistedEMA()) {
				Coms.send("[CMD=RESET]: REMOVED EMA");
			}
			if (rmKVpersistedWeights()) {
				Coms.send("[CMD=RESET]: REMOVED W_out");
			}
			break;
		}
		case CMD_SHARE_WEIGHTS:
			// TODO: case CMD_SHARE_WEIGHTS:
			break;
		case CMD_PERSIST: {
			Coms.send("[CMD=PERSIST]: INIT");
			persistOutputWeights();
			Coms.send("[CMD=PERSIST]: WEIGHTS PERSISTED");
			persistEMA();
			Coms.send("[CMD=PERSIST]: EMAs PERSISTED");
			communicatePersistance();
			Coms.send("[CMD=PERSIST]: DONE");
			break;
		}
	}
}
