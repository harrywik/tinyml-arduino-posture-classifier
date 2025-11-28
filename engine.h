#pragma once
#include "button.h"

void buttonHandler(void);

void runIteration(CommunicationMode mode);

enum OperationMode {
	IDLE,
	COLLECTION,
	TRAINING,
	VALIDATION,
	WEIGHT_SHARING,
	INFERENCE
};
