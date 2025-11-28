#pragma once
// These are the standard Mbed OS return codes for success and common errors.
#define KV_R_OK                 0   // Success
#define KV_R_NO_KEY             -3  // Key not found
#define KV_R_ERROR              -1  // Generic error 
#define KV_R_WRITE_ERROR        -10 // Write failure

// Simple get/set
bool getKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]);
bool setKVPersistedWeights(float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]);
