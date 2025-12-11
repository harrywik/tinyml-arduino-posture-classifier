#pragma once
#include <Arduino.h>
#include "imu_features.h"

#define RESERVOIR_SIZE 20    // Number of reservoir neurons
#define OUTPUT_SIZE 3        // Number of classes: Sitting, Standing, Moving
#define INPUT_SIZE NUM_FEATURES
#define LEAKY 0.3           // Leaky integration parameter
#define LEARNING_RATE 0.01  // Learning rate for output layer training

struct ESN {
    float reservoir[RESERVOIR_SIZE];
    float W_in[RESERVOIR_SIZE][INPUT_SIZE];   // Input weights
    float W_res[RESERVOIR_SIZE][RESERVOIR_SIZE]; // Reservoir recurrent weights
    float W_out[OUTPUT_SIZE][RESERVOIR_SIZE]; // Output weights (trainable)
};

typedef struct {
    float weights[OUTPUT_SIZE][RESERVOIR_SIZE];
} shareableWeights;


void initESN();

void updateReservoir(const FeatureVector& fv);

void trainOutputLayer(const FeatureVector* X, const uint8_t* y, uint16_t n_samples, float learning_rate = LEARNING_RATE);

shareableWeights getW_out(void);

void setW_out(shareableWeights W_new);

uint8_t predict();

void persistOutputWeights(void);
