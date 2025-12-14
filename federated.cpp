#include "io.h"
#include "ble.h"
#include "esn.h"

bool shareW_out(uint16_t* nBatchesOnDevice) {
	shareableWeights W_a = getW_out();
	shareableWeights W_b;
	uint16_t n_a, n_b, n_tot;
	n_a = *nBatchesOnDevice;

	uint16_t W_out_length = OUTPUT_SIZE * RESERVOIR_SIZE;

	if (Coms.getUUID()) {
		// THIS DEV IS CENTRAL
		// first send
		if (!Coms.sendModel((float*) W_a.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.sendModel()");
			return false;
		}
		if (!Coms.sendNBatches((const uint16_t) n_a, sizeof(uint16_t))) {
			Serial.println("Failed on Coms.sendNBatches()");
			return false;
		}

		// then receive
		if (!Coms.receiveModel((float*) W_b.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.receiveModel()");
			return false;
		}

		if (!Coms.receiveNBatches(&n_b, sizeof(uint16_t))) {
			Serial.println("Failed on Coms.receiveNBatches()");
			return false;
		}

		// RETURN TO PRIOR STATE
		deinitAsCentral();
	} else {
		// THIS DEV IS PERIPHERAL
		// first receive
		if (!Coms.receiveModel((float*) W_b.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.receiveModel()");
			return false;
		}

		if (!Coms.receiveNBatches(&n_b, sizeof(uint16_t)))  {
			Serial.println("Failed on Coms.receiveNBatches()");
			return false;
		}

		// then send
		if (!Coms.sendModel((float*) W_a.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.sendModel()");
			return false;
		}

		if (!Coms.sendNBatches((const uint16_t) n_a, sizeof(uint16_t))) {
			Serial.println("Failed on Coms.sendNBatches()");
			return false;
		}

	}

	n_tot = n_a + n_b;

	for(size_t i = 0; i < OUTPUT_SIZE; i++) {
		for(size_t j = 0; j < RESERVOIR_SIZE; j++) {
			W_a.weights[i][j] = (W_a.weights[i][j] * n_a + W_b.weights[i][j] * n_b) / n_tot;
		}
	}
	*nBatchesOnDevice = n_tot;
	setW_out(W_a);
       	return true;
}
