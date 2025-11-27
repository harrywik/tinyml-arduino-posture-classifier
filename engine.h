#pragma once

void runIteration(void);

enum OperationMode {
	IDLE,
	COLLECTION,
	TRAINING,
	VALIDATION,
	WEIGHT_SHARING,
	INFERENCE
};
