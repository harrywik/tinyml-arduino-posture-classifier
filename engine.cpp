#include "esn.h"
#include "engine.h"
#include "button.h"
#include "imu_features.h"
#include "serial_protocol.h"

OperationMode mode = IDLE;
FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

volatile bool interrupt = false;

void buttonHandler(void) {
	interrupt = true;
}

void runIteration(CommunicationMode mode) {

	bool flag = false;

    	// Disable interrupts while reading/clearing the shared variable
    	__disable_irq();
    	if (interrupt) {
        	flag = true;
        	interrupt = false;
    	}
    	__enable_irq();
	
	if (flag)
		Serial.println("triggered");
	//
	// Button press after startup
	if (flag && !buttonPressIgnore()) {
		// Perform action
		persistOutputWeights();
		persistEMA();
		// Communicate
		communicatePersistance();
	}

	// TODO:
	// implement BLE
	// wrap all calls to Serial with an IO-class instead
	// caller agnostic to whether BLE or USB
	// is the chosen communication mode
	if (mode == BLE) return; // for now return early

	SerialCommandType order = readSerialCommand();
	switch (order) {
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
