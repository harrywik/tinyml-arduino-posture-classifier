#include "KVStore.h"
#include "kvstore_global_api.h"
#include "esn.h"
#include "persist_weights.h"

const char* const W_OUT_KEY = "W_out_key";
const size_t W_OUT_BYTES = sizeof(float) * OUTPUT_SIZE * RESERVOIR_SIZE;

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
