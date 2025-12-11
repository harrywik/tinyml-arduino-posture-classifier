#include "io.h"
#include "ble.h"
#include "esn.h"

bool shareW_out(uint16_t* nBatchesOnDevice) {
	shareableWeights W_a = getW_out();
	shareableWeights W_b;
	uint16_t n_a, n_b, n_tot;
	n_a = *nBatchesOnDevice;

	W_out_length = OUTPUT_SIZE * RESERVOIR_SIZE;
	if (Coms.getUUID()) {
		// THIS DEV IS CENTRAL
		// first send
		Coms.sendModel((const float*) W_a, sizeof(float) * W_out_length);
		Coms.sendNBatches((const uint16_t) n_a, sizeof(uint16_t));
		// then receive
		Coms.receiveModel(W_b, sizeof(float) * W_out_length);
		Coms.receiveNBatches(&n_b, sizeof(uint16_t));
		// RETURN TO PRIOR STATE
		deinitAsCentral();
	} else {
		// THIS DEV IS PERIPHERAL
		// first receive
		Coms.receiveModel(W_b, sizeof(float) * W_out_length);
		Coms.receiveNBatches(&n_b, sizeof(uint16_t));
		// then send
		Coms.sendModel((const float*) W_a, sizeof(float) * W_out_length);
		Coms.sendNBatches((const uint16_t) n_a, sizeof(uint16_t));
	}

	n_tot = n_a + n_b;

	for(size_t i = 0; i < OUTPUT_SIZE; i++) {
		for(size_t j = 0; j < RESERVOIR_SIZE; j++) {
			W_a[i][j] = W_a[i][j]*n_a + W_b[i][j]*n_b / n_tot;
		}
	}
	*nBatchesOnDevice = n_tot;
	setW_out(W_a);
       	return true;
}
