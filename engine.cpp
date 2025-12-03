#include "io.h"
#include "led.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "data_utils.h"
#include "persistance.h"
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
				break;
			}

			bool success = KVappendCollected(windowBuffer, labelsBuffer);
			if (success)
				Coms.send("[CMD=COLLECT]: PERSISTED");
			else
				Coms.send("[CMD=COLLECT]: FAILED TO PERSIST");
			break;
		}
		case CMD_TRAIN: {
			Coms.send("[CMD=TRAIN]: INIT");

			Coms.send("[CMD=TRAIN]: CALC TRAIN/VAL SPLIT");
			uint16_t n;
			if(!get_n_total(&n)) {
				Coms.send("[CMD=TRAIN]: FAILED");
			}

			std::vector<uint16_t> train_idxs, val_idxs;
			train_val_split_indices_deterministic(n, TRAIN_RATIO, SEED, train_idxs, val_idxs);
			Coms.send("[CMD=TRAIN]: SPLIT DONE.");
			Coms.send("[CMD=TRAIN]: INIT AVGs & VARs");
			if (initNormalization(train_idxs))
				Coms.send("[CMD=TRAIN]: INIT DONE.");
			else {
				Coms.send("[CMD=TRAIN]: INIT FAILED");
				break;
			}

			for (uint16_t i : train_idxs) {
				Coms.send("[CMD=TRAIN]: KV FETCH");
				if (getCollectedWindow(windowBuffer, labelsBuffer, i))
					Coms.send("[CMD=TRAIN]: FETCH DONE");
				else {
					Coms.send("[CMD=TRAIN]: FETCH FAILED");
					return;
				}

				Coms.send("[CMD=TRAIN]: NORMALIZATION");
				normalizeWindow(windowBuffer, nSamples);
				Coms.send("[CMD=TRAIN]: GRADIENT DESCENT");
				trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
			}
			// Flush reservoir by reinit
			initESN();
			for (size_t i : val_idxs) {
				// validate()
			}
			// Flush reservoir by reinit
			initESN();
			/*
			size_t i = 0;
			Coms.send("[CMD=TRAIN]: PRINTING PREDICTIONS:");
			while (nSamples--) {
				updateReservoir(windowBuffer[i++]);
				uint8_t prediction = predict();
				Coms.send(String(prediction));
			}
			*/
			nSamples = 0;
			break;
		}
		case CMD_VAL: {
			// normalizeWindow(windowBuffer, nSamples);
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
		case CMD_RESET: {
			Coms.send("[CMD=RESET]: INIT");
			/*
			if (rmKVpersistedEMA()) {
				Coms.send("[CMD=RESET]: REMOVED EMA");
			}
			*/
			if (resetKV()) {
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
			// persistEMA();
			// Coms.send("[CMD=PERSIST]: EMAs PERSISTED");
			communicatePersistance();
			Coms.send("[CMD=PERSIST]: DONE");
			break;
		}
	}
}
