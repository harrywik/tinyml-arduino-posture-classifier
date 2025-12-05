#pragma once
#include <Arduino.h>
#include <Arduino_LSM9DS1.h> // IMU library for Nano 33 BLE Sense
#include "engine.h" // BATCH_SIZE definition

#define WINDOW_SIZE 64     // Number of samples per window
#define NUM_FEATURES 12    // 6 from accel (mean+std) + 6 from gyro (mean+std)
#define EMA_ALPHA 0.1      // For exponential moving average (TUNABLE)

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
void collectWindow(FeatureVector (&window)[BATCH_SIZE], uint16_t *nSamples);

// Calibrate the EMA during training
void updateEMA(FeatureVector (&windowBuffer)[BATCH_SIZE], uint16_t nSamples);

// Normalize the collected window
void normalizeWindow(FeatureVector (&windowBuffer)[BATCH_SIZE], uint16_t nSamples);

void persistEMA(void);

bool IMUbufferReady(void);
