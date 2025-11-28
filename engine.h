#pragma once
#include "button.h"

void runIteration(CommunicationMode mode);

void handleBLECommands(void);

void handleUSBCommands(void);

enum OperationMode {
	IDLE,
	COLLECTION,
	TRAINING,
	VALIDATION,
	WEIGHT_SHARING,
	INFERENCE
};
