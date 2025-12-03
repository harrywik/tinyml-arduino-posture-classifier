V#include <string.h>
#include <format.h>
#include "KVStore.h"
#include "kvstore_global_api.h"
#include "esn.h"
#include "persistance.h"

const char* const W_OUT_KEY = "W_out_key";
const size_t W_OUT_BYTES = sizeof(float) * OUTPUT_SIZE * RESERVOIR_SIZE;
const char* const N_TOTAL = "N";
const size_t SIZEOF_N_TOTAL = sizeof(uint16_t);


// Return success
bool setKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]) {
    	// kv_set requires the data address to be cast to a uint8_t pointer (raw bytes).
    	// The array name 'W_out' decays to the address of the first element.
    	size_t ret = kv_set(W_OUT_KEY, (const uint8_t*) W_out, W_OUT_BYTES, 0);

    	if (ret == KV_R_OK)
        	return true;
        return false;
}

// Return success here too
bool getKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]) {
	size_t actual_size;
    	size_t ret = kv_get(W_OUT_KEY, (uint8_t*) W_out, W_OUT_BYTES, &actual_size);

    	if (ret == KV_R_OK && actual_size == W_OUT_BYTES)
        	return true;
        return false;
}

bool rmKVpersistedWeights(void) {
	return kv_remove(W_OUT_KEY) == KV_R_OK;
}

bool KVappendCollected(
	FeatureVector windowBuffer[WINDOW_SIZE], 
	uint8_t labelsBuffer[WINDOW_SIZE]
) {
	uint16_t n_total;
	size_t actual_size;
    	size_t ret = kv_get(N_TOTAL, (uint8_t*) &n_total, SIZEOF_N_TOTAL, &actual_size);

	if (ret != KV_R_OK || actual_size != SIZEOF_N_TOTAL) {
		// Not previously stored
		n_total = 0;
	}
	std::string labelKey = std::format("l{}", n_total);
	// Set label 
	// All labelsBuffer elements should contain the same value
	ret = kv_set(labelKey.c_str(), &labelsBuffer[0], 1, 0);
	if (ret != KV_R_OK)
		return false;

	float featVec[WINDOW_SIZE];
	for (size_t j = 0; j < NUM_FEATURES; j++) {
		for (size_t i = 0; i < WINDOW_SIZE; i++) {
			featVec[i] = windowBuffer[i].features[j];
		}
		std::string featureKey = std::format("d{}f{}", n_total, j);
		ret = kv_set(featureKey.c_str(), (const uint8_t*) featVec, sizeof(featVec), 0);
		if (ret != KV_R_OK)
			return false;
	}

	n_total++;
	ret = kv_set(N_TOTAL, (const uint8_t*) &n_total, SIZEOF_N_TOTAL, 0);
	return ret == KV_R_OK;
}
