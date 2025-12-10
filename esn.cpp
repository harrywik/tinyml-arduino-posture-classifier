#include "esn.h"
#include "persistance.h"
#include <math.h>
#include <stdlib.h>  // for rand()

static ESN esn;

// Initialize the ESN
void initESN() {
    // Seed for reproducability
    srand(42); 

    // Zero reservoir state
    for (uint8_t i = 0; i < RESERVOIR_SIZE; i++) {
        esn.reservoir[i] = 0.0f;
    }

    // Random input weights (-1 to 1)
    for (uint8_t i = 0; i < RESERVOIR_SIZE; i++) {
        for (uint8_t j = 0; j < INPUT_SIZE; j++) {
            esn.W_in[i][j] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        }
    }

    // Random sparse reservoir weights (-0.5 to 0.5)
    for (uint8_t i = 0; i < RESERVOIR_SIZE; i++) {
        for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
            esn.W_res[i][j] = ((float)rand() / RAND_MAX) - 0.5f;
        }
    }
    
    // TODO:
    // Log if we have errors in KVStore

    if (getKVPersistedWeights(esn.W_out))
	// We got a succesfull retrieval
	return;

    // Or not...
    // Zero output weights (trainable)
    for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
        for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
            esn.W_out[i][j] = 0.0f;
        }
    }
}

shareableWeights getW_out(void) {
	return esn.W_out;
}

void setW_out(shareableWeights W_new) {
	memcpy(W_new, esn.W_out, sizeof(esn.W_out));
}

void persistOutputWeights(void) {
	// TODO:
	// log any errors
	setKVPersistedWeights(esn.W_out);
}

// Update the reservoir state with a new feature vector
void updateReservoir(const FeatureVector& fv) {
    float new_state[RESERVOIR_SIZE];

    for (uint8_t i = 0; i < RESERVOIR_SIZE; i++) {
        float sum = 0.0f;

        // Input contribution
        for (uint8_t j = 0; j < INPUT_SIZE; j++) {
            sum += esn.W_in[i][j] * fv.features[j];
        }

        // Reservoir recurrent contribution
        for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
            sum += esn.W_res[i][j] * esn.reservoir[j];
        }

        // Apply tanh activation
        new_state[i] = (1 - LEAKY) * esn.reservoir[i] + LEAKY * tanh(sum);
    }

    // Update reservoir state
    for (uint8_t i = 0; i < RESERVOIR_SIZE; i++) {
        esn.reservoir[i] = new_state[i];
    }
}

// Train output layer with simple online gradient descent
void trainOutputLayer(const FeatureVector* X, const uint8_t* y, uint16_t n_samples, float learning_rate) {
    for (uint16_t s = 0; s < n_samples; s++) {
        updateReservoir(X[s]);

        // Compute output (linear)
        float out[OUTPUT_SIZE];
        for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
            out[i] = 0.0f;
            for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
                out[i] += esn.W_out[i][j] * esn.reservoir[j];
            }
        }

        // Softmax
        float max_val = out[0];
        for (uint8_t i = 1; i < OUTPUT_SIZE; i++) if (out[i] > max_val) max_val = out[i];
        float sum_exp = 0.0f;
        for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
            out[i] = exp(out[i] - max_val);
            sum_exp += out[i];
        }
        for (uint8_t i = 0; i < OUTPUT_SIZE; i++) out[i] /= sum_exp;

        // Compute error (one-hot)
        for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
            float target = (i == y[s]) ? 1.0f : 0.0f;
            float error = target - out[i];

            // Gradient descent update for W_out
            for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
                esn.W_out[i][j] += learning_rate * error * esn.reservoir[j];
            }
        }
    }
}

// Predict class label from current reservoir state
uint8_t predict() {
    float out[OUTPUT_SIZE] = {0};

    for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
        for (uint8_t j = 0; j < RESERVOIR_SIZE; j++) {
            out[i] += esn.W_out[i][j] * esn.reservoir[j];
        }
    }

    // Return argmax
    uint8_t max_idx = 0;
    float max_val = out[0];
    for (uint8_t i = 1; i < OUTPUT_SIZE; i++) {
        if (out[i] > max_val) {
            max_val = out[i];
            max_idx = i;
        }
    }
    return max_idx;
}

