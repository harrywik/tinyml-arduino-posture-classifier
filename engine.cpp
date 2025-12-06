#include "io.h"
#include "led.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "persistance.h"
#include "imu_features.h"
// #include "eval.h"

// True labels remain constant for a batch 
FeatureVector featureBuffer[BATCH_SIZE];
// Label buffer is also used for predictions and may not
uint8_t labelBuffer[BATCH_SIZE];

uint16_t nSamples = 0;

void runIteration(void) {
	SerialCommandType order = Coms.receive();

	switch (order) {
		case CMD_NONE:
			updateIMU();
			if (IMUwindowReady()) {
				// No IO here should be quick
				FeatureVector state = computeFeatures();
				updateEMA(state);
				normalizeVector(state);
				updateReservoir(state);
			}
			return;
		case CMD_TRAIN: {
			Coms.send("[CMD=TRAIN]: INIT");
			// This will set nSamples
			collectBuffer(featureBuffer, &nSamples);
			Coms.send("[CMD=TRAIN]: COLLECTED");
			// This will set equally many labels
			if (!Coms.getLabel(labelBuffer, nSamples)) {
				Coms.send("Bad input");
				nSamples = 0;
				break;
			}
			Coms.send("[CMD=TRAIN]: NORMALIZATION");
			normalizeBuffer(featureBuffer, nSamples);
			Coms.send("[CMD=TRAIN]: GRADIENT DESCENT");
			trainOutputLayer(featureBuffer, labelBuffer, nSamples, LEARNING_RATE);
			size_t i = 0;
			Coms.send("[CMD=TRAIN]: PRINTING PREDICTIONS:");
			while (nSamples--) {
				updateReservoir(featureBuffer[i++]);
				uint8_t prediction = predict();
				Coms.send(String(prediction));
			}
			nSamples = 0;
			Coms.send("[CMD=TRAIN]: DONE");
			break;
		}
		/*
		case CMD_VAL: {
			evaluateLoop();
			break;
		}
		case CMD_VAL_DONE:{
			printResults();
			break;
		}
		*/
		case CMD_INFER: {
			Coms.send("[CMD=INFER]: INIT");
			collectBuffer(featureBuffer, &nSamples);
			normalizeBuffer(featureBuffer, nSamples);
			size_t i = 0;
			while (nSamples--) {
				updateReservoir(featureBuffer[i++]);
				uint8_t prediction = predict();
				Coms.send(String(prediction));
			}
			nSamples = 0;
			Coms.send("[CMD=INFER]: DONE");
			break;
		}
		case CMD_RESET: {
			Coms.send("[CMD=RESET]: INIT");
			// Uncomment when eval works
			// resetMetrics();
			Coms.send("[CMD=RESET]: FLUSHED EVAL");
			if (rmKVpersistedEMA()) {
				Coms.send("[CMD=RESET]: REMOVED EMA");
			}
			if (rmKVpersistedWeights()) {
				Coms.send("[CMD=RESET]: REMOVED W_out");
			}
			Coms.send("[CMD=RESET]: DONE");
			break;
		}
		case CMD_SHARE_WEIGHTS: {
			// TODO: case CMD_SHARE_WEIGHTS:
			break;
		}
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
