#pragma once
#include <Arduino.h>
#include <Arduino_LSM9DS1.h> // IMU library for Nano 33 BLE Sense

#define WINDOW_SIZE 128    // Number of samples per window
#define NUM_FEATURES 12    // 6 from accel (mean+std) + 6 from gyro (mean+std)

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
void collectWindow(FeatureVector (&window)[WINDOW_SIZE]);
