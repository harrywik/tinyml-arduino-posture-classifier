#include "KVStore.h"
#include "kvstore_global_api.h"
#include "esn.h"
#include "persistance.h"

const char* const W_OUT_KEY = "W_out_key";
const size_t W_OUT_BYTES = sizeof(float) * OUTPUT_SIZE * RESERVOIR_SIZE;


const char* const EMA_KEY = "EMA_key";
const size_t EMA_BYTES = sizeof(float) * NUM_FEATURES;


const char* const BATCH_KEY = "nBATCH_key";
const size_t BATCH_N_SIZE = sizeof(uint16_t);


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

// Similarily for EMAs
bool setKVPersistedEMA(float EMAs[NUM_FEATURES]) {
    	size_t ret = kv_set(EMA_KEY, (const uint8_t*) EMAs, EMA_BYTES, 0);

    	if (ret == KV_R_OK)
        	return true;
        return false;
}

bool getKVPersistedEMA(float EMAs[NUM_FEATURES]) {
	size_t actual_size;
    	size_t ret = kv_get(EMA_KEY, (uint8_t*) EMAs, EMA_BYTES, &actual_size);

    	if (ret == KV_R_OK && actual_size == EMA_BYTES)
        	return true;
        return false;
}

bool rmKVpersistedEMA(void) {
	return kv_remove(EMA_KEY) == KV_R_OK;
}

// Fetch current current number of batches processed
bool getNProcessedBatches(uint16_t* nBatches) {
	size_t actual_size;
    	size_t ret = kv_get(BATCH_KEY, (uint8_t*) nBatches, BATCH_N_SIZE, &actual_size);
    	if (ret == KV_R_OK && actual_size == BATCH_N_SIZE)
		return true;

	*nBatches = 0;
    	ret = kv_set(BATCH_KEY, (const uint8_t*) nBatches, BATCH_N_SIZE, 0);
    	return ret == KV_R_OK;

}
// Increment from current number of batches processed
bool incNProcessedBatches(uint16_t increment) {
	uint16_t current;
	if (!getNProcessedBatches(&current))
		return false;

	uint16_t updated = current + increment;
    	size_t ret = kv_set(BATCH_KEY, (const uint8_t*) &updated, BATCH_N_SIZE, 0);

    	return ret == KV_R_OK;
}
bool rmNProcessedBatches(void) {
	return kv_remove(BATCH_KEY) == KV_R_OK;
}
