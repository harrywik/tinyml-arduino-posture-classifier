#include "KVStore.h"
#include "kvstore_global_api.h"
#include "io.h"
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

bool resetKV(void) {
    uint16_t n_total;
    size_t actual_size;
    size_t ret;
    
    // 1. --- Retrieve the total count ---
    // Read the counter to know the upper limit of the collected data indices (0 to n_total - 1)
    ret = kv_get(N_TOTAL, (uint8_t*) &n_total, SIZEOF_N_TOTAL, &actual_size);

    // If the count key isn't found, assume n_total is 0 and proceed (it means the store is already empty of collected data)
    if (ret != KV_R_OK || actual_size != SIZEOF_N_TOTAL) {
        n_total = 0;
    }
    
    bool overall_success = true;

    // 2. --- Delete all Label and Feature Keys ---
    for (uint16_t i = 0; i < n_total; i++) {
        // A. Delete Label Key (l{i})
        std::stringstream ss_label;
        ss_label << "l" << i;
        std::string labelKey = ss_label.str();
        
        // We delete it regardless of whether it's found (delete often returns a success code 
        // even if the key is not present, but we must check for permanent errors)
        ret = kv_reset(labelKey.c_str());
        if (ret != KV_R_OK && ret != KV_R_NO_KEY) { // Check for a critical error
            overall_success = false;
        }

        // B. Delete all Feature Keys for this index (d{i}f{j})
        for (size_t j = 0; j < NUM_FEATURES; j++) {
            std::stringstream ss_feature;
            ss_feature << "d" << i << "f" << j;
            std::string featureKey = ss_feature.str();

            ret = kv_reset(featureKey.c_str());
            if (ret != KV_R_OK && ret != KV_R_NO_KEY) { // Check for a critical error
                overall_success = false;
            }
        }
    }

    // 3. --- Delete the persistent counter key itself ---
    ret = kv_reset(N_TOTAL);
    if (ret != KV_R_OK && ret != KV_R_NO_KEY) {
        overall_success = false;
    }

    return overall_success;
}

bool getCollectedWindow(
	FeatureVector (&windowBuffer)[WINDOW_SIZE], 
	uint8_t (&labelsBuffer)[WINDOW_SIZE],
	uint16_t index
) {
	std::stringstream ss;
	ss << "l" << index;
	std::string labelKey = ss.str();

	uint8_t label;
	size_t actual_size;
    	size_t ret = kv_get(labelKey.c_str(), (uint8_t*) &label, 1, &actual_size);

	if (ret != KV_R_OK || actual_size != 1)
		return false;

	for (size_t i = 0; i < WINDOW_SIZE; i++) {
		labelsBuffer[i] = label;
	}


	float featVec[WINDOW_SIZE];
	for (size_t i = 0; i < NUM_FEATURES; i++) {
		std::stringstream ss;
		ss << "d" << index << "f" << i;
		std::string dataKey = ss.str();
		size_t expected_size = sizeof(featVec);
		ret = kv_get(dataKey.c_str(), (uint8_t*) featVec, expected_size, &actual_size);

		if (ret != KV_R_OK || actual_size != expected_size)
            		return false;
		for (size_t j = 0; j < WINDOW_SIZE; j++) {
			    windowBuffer[j].features[i] = featVec[j];
		}
        }
	return true;
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
		Coms.send("1st false setting n_total to 0");
	}

	std::stringstream ss;
	ss << "l" << n_total;
	std::string labelKey = ss.str();
	
	// Set label 
	// All labelsBuffer elements should contain the same value
	ret = kv_set(labelKey.c_str(), &labelsBuffer[0], 1, 0);
	if (ret != KV_R_OK) {
		Coms.send("2nd false");
		return false;
	}

	float featVec[WINDOW_SIZE];
	for (size_t j = 0; j < NUM_FEATURES; j++) {
		for (size_t i = 0; i < WINDOW_SIZE; i++) {
			featVec[i] = windowBuffer[i].features[j];
		}
		std::stringstream ss;
		ss << "d" << n_total << "f" << j;
		std::string featureKey = ss.str();

		ret = kv_set(featureKey.c_str(), (const uint8_t*) featVec, sizeof(featVec), 0);
		if (ret != KV_R_OK) {
			Coms.send("3rd false");
			return false;
		}
	}

	n_total++;
	ret = kv_set(N_TOTAL, (const uint8_t*) &n_total, SIZEOF_N_TOTAL, 0);
	return ret == KV_R_OK;
}

bool get_n_total(uint16_t* n) {
	uint16_t n_total;
	size_t actual_size;
    	size_t ret = kv_get(N_TOTAL, (uint8_t*) &n_total, SIZEOF_N_TOTAL, &actual_size);

	if (ret != KV_R_OK || actual_size != SIZEOF_N_TOTAL) {
		return false;
	}
	*n = n_total;
	return true;
}

bool calcNormalizationParams(
    float AVGs[NUM_FEATURES], 
    float VARs[NUM_FEATURES], 
    const std::vector<uint16_t>& train_idxs // Pass by const reference
) {
    // We calculate N_TOTAL once here
    const size_t N_TOTAL = train_idxs.size() * WINDOW_SIZE;

    // Check for division by zero
    if (N_TOTAL == 0) {
        // Handle case where training set is empty
        return true; 
    }

    float featVec[WINDOW_SIZE];
    size_t actual_size;
    int ret;

    // Loop over each feature to calculate its mean and variance independently
    for (size_t j = 0; j < NUM_FEATURES; j++) {
        // Must be initialized OUTSIDE the sample loop to accumulate globally
        double global_sum = 0.0;
        double global_sum_sq = 0.0;
        
        // Loop over each training sample index
        for (size_t i : train_idxs) {
            
	    std::stringstream ss;
	    ss << "d" << i << "f" << j;
	    std::string featureKey = ss.str();
            
            ret = kv_get(featureKey.c_str(), (uint8_t*) featVec, sizeof(featVec), &actual_size);
            
            if (ret != KV_R_OK || actual_size != sizeof(featVec))
                return false;

            for (size_t k = 0; k < WINDOW_SIZE; k++) {
                global_sum += featVec[k];
                global_sum_sq += featVec[k] * featVec[k];
            }
        }
        
        // Global Mean (mu)
        float mu = static_cast<float>(global_sum / N_TOTAL);
        
        float E_X_sq = static_cast<float>(global_sum_sq / N_TOTAL);
        float sigma_sq = E_X_sq - (mu * mu);

        // Ensure variance is non-negative due to floating-point error
        if (sigma_sq < 0.0f) sigma_sq = 0.0f;

        AVGs[j] = mu;
        VARs[j] = sigma_sq; 
    }

    return true;
}
