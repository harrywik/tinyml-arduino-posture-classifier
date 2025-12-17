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
		// Wait for peripheral to be ready to receive
		Serial.println("Central: waiting for peripheral to be ready...");
		delay(5000);

		// Validate data before sending
		Serial.print("Central: First few W_a values to send: ");
		Serial.print(W_a.weights[0][0]); Serial.print(", ");
		Serial.print(W_a.weights[0][1]); Serial.print(", ");
		Serial.println(W_a.weights[0][2]);

		// first send
		if (!Coms.sendModel((float*) W_a.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.sendModel()");
			return false;
		}

		// Wait for peripheral to receive the model before sending batch count
		Serial.println("Central: waiting before sending batch count...");
		delay(5000);

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
		// Initialize W_b to zeros to detect receive failures
		memset(W_b.weights, 0, sizeof(W_b.weights));

		// Wait for central to connect and be ready to send
		// Central delays 5s + connection time, so peripheral should wait longer
		Serial.println("Peripheral: waiting for central to connect and prepare...");
		delay(8000);

		// first receive
		if (!Coms.receiveModel((float*) W_b.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.receiveModel()");
			return false;
		}

		// Validate received data
		Serial.print("Peripheral: First few W_b values: ");
		Serial.print(W_b.weights[0][0]); Serial.print(", ");
		Serial.print(W_b.weights[0][1]); Serial.print(", ");
		Serial.println(W_b.weights[0][2]);

		if (!Coms.receiveNBatches(&n_b, sizeof(uint16_t)))  {
			Serial.println("Failed on Coms.receiveNBatches()");
			return false;
		}

		// Wait for central to be ready to receive before sending
		Serial.println("Peripheral: waiting for central to be ready...");
		delay(3000);

		// then send
		if (!Coms.sendModel((float*) W_a.weights, sizeof(float) * W_out_length)) {
			Serial.println("Failed on Coms.sendModel()");
			return false;
		}

		// Wait for central to receive the model before sending batch count
		Serial.println("Peripheral: waiting before sending batch count...");
		delay(8000);

		if (!Coms.sendNBatches((const uint16_t) n_a, sizeof(uint16_t))) {
			Serial.println("Failed on Coms.sendNBatches()");
			return false;
		}

	}

	n_tot = n_a + n_b;
	float new_weight, old_weight;

	Serial.print("n_a: ");
	Serial.print(n_a);
	Serial.print(", n_b: ");
	Serial.println(n_b);

	if (n_tot == 0)
		// Exit early to avoid div by zero
		return false;


	for(size_t i = 0; i < OUTPUT_SIZE; i++) {
		for(size_t j = 0; j < RESERVOIR_SIZE; j++) {
			old_weight = W_a.weights[i][j];
			
			W_a.weights[i][j] = (W_a.weights[i][j] * (float) n_a + W_b.weights[i][j] * (float) n_b) / (float) n_tot;
			new_weight = W_a.weights[i][j];

			Serial.print("i: ");
			Serial.print(i);
			Serial.print(" j: ");
			Serial.print(j);
			Serial.print(" | ");
			Serial.print("W_a.weight:");
			Serial.print(old_weight);
			Serial.print(", W_b.weight:");
			Serial.print(W_b.weights[i][j]);
			// Serial.print(", old_weight:");
			// Serial.print(old_weight);
			Serial.print(", new_weight:");
			Serial.println(new_weight);
		}
	}
	*nBatchesOnDevice = n_tot;
	setW_out(W_a);
    return true;
}
