#pragma once
#include <math.h>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include "esn.h"
#include "imu_features.h"
// These are the standard Mbed OS return codes for success and common errors.
#define KV_R_OK                 0   // Success
#define KV_R_NO_KEY             -3  // Key not found
#define KV_R_ERROR              -1  // Generic error 
#define KV_R_WRITE_ERROR        -10 // Write failure

// Simple get/set
// W_out
bool getKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]);
bool setKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]);
bool resetKV(void);

// For the collected data
bool KVappendCollected(
	FeatureVector windowBuffer[WINDOW_SIZE], 
	uint8_t labelsBuffer[WINDOW_SIZE]
);

// Get number of Windows persisted
bool get_n_total(uint16_t* n);

// For IMU features
bool calcNormalizationParams(
    float AVGs[NUM_FEATURES], 
    float VARs[NUM_FEATURES], 
    const std::vector<uint16_t>& train_idxs
);

bool getCollectedWindow(
	FeatureVector (&windowBuffer)[WINDOW_SIZE], 
	uint8_t (&labelsBuffer)[WINDOW_SIZE],
	uint16_t index
);
