#pragma once
#include <Arduino.h>
#include <Arduino_LSM9DS1.h> // IMU library for Nano 33 BLE Sense
#include "engine.h" // BATCH_SIZE definition

#define WINDOW_SIZE 128    // Number of samples per window
#define NUM_FEATURES 18    // 6 from accel (mean+std) + 6 from gyro (mean+std) + 6 from mag (mean+std)
#define EMA_ALPHA 0.005     // For exponential moving average (TUNABLE)

struct FeatureVector {
    float features[NUM_FEATURES];
};

// Initialize the IMU
bool initIMU();

// Update IMU buffer with new sample
void updateIMU();

// Compute features from the sliding window
FeatureVector computeFeatures();

// Collect a window to later be sent to storage
void collectBuffer(FeatureVector (&featureBuffer)[BATCH_SIZE], uint16_t *nSamples);

// Calibrate the EMA during training
void updateEMA(FeatureVector vector);

// Normalize the entire collected buffer
void normalizeBuffer(FeatureVector (&featureBuffer)[BATCH_SIZE], uint16_t nSamples);

// Normalize a single vector
void normalizeVector(FeatureVector &vector);

// Persist the EMA state
void persistEMA(void);

// Check if we can update reservoir
bool IMUwindowReady(void);
