#pragma once
#include <Arduino.h>
#include "imu_features.h"

#define RESERVOIR_SIZE 20    // Number of reservoir neurons
#define OUTPUT_SIZE 4        // Number of classes: Sitting, Standing, Lying, Active
#define INPUT_SIZE NUM_FEATURES
#define LEAKY 0.3           // Leaky integration parameter

struct ESN {
    float reservoir[RESERVOIR_SIZE];
    float W_in[RESERVOIR_SIZE][INPUT_SIZE];   // Input weights
    float W_res[RESERVOIR_SIZE][RESERVOIR_SIZE]; // Reservoir recurrent weights
    float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]; // Output weights (trainable)
};

void initESN();
void updateReservoir(const FeatureVector& fv);
void trainOutputLayer(const FeatureVector* X, const uint8_t* y, uint16_t n_samples, float learning_rate = 0.01);
uint8_t predict();

