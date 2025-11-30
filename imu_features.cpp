#include "imu_features.h"
#include "persistance.h"
#include <math.h>

static float accelX[WINDOW_SIZE];
static float accelY[WINDOW_SIZE];
static float accelZ[WINDOW_SIZE];

static float gyroX[WINDOW_SIZE];
static float gyroY[WINDOW_SIZE];
static float gyroZ[WINDOW_SIZE];

static float EMAs[NUM_FEATURES] = {0.0};

static uint8_t sampleIndex = 0;
static bool bufferFilled = false;

// Initialize IMU sensor
bool initIMU() {
    	if (!IMU.begin()) {
        	return false;
    	}
	// Load EMAs if available
    	getKVPersistedEMA(EMAs);
    	return true;
}


// Add new sample to the circular buffer
void updateIMU() {
    float ax, ay, az, gx, gy, gz;

    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
        IMU.readAcceleration(ax, ay, az);
        IMU.readGyroscope(gx, gy, gz);

        accelX[sampleIndex] = ax;
        accelY[sampleIndex] = ay;
        accelZ[sampleIndex] = az;

        gyroX[sampleIndex] = gx;
        gyroY[sampleIndex] = gy;
        gyroZ[sampleIndex] = gz;

        sampleIndex++;
        if (sampleIndex >= WINDOW_SIZE) {
            sampleIndex = 0;
            bufferFilled = true;
        }
    }
}

// Compute mean and standard deviation for an array
static void computeMeanStd(const float* data, uint8_t size, float& mean, float& stddev) {
    mean = 0.0;
    for (uint8_t i = 0; i < size; i++) {
        mean += data[i];
    }
    mean /= size;

    stddev = 0.0;
    for (uint8_t i = 0; i < size; i++) {
        float diff = data[i] - mean;
        stddev += diff * diff;
    }
    stddev = sqrt(stddev / size);
}

// Compute feature vector
FeatureVector computeFeatures() {
    FeatureVector fv;

    uint8_t count = bufferFilled ? WINDOW_SIZE : sampleIndex;

    float mean, stddev;
    uint8_t idx = 0;

    // Accel X/Y/Z
    computeMeanStd(accelX, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    computeMeanStd(accelY, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    computeMeanStd(accelZ, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    // Gyro X/Y/Z
    computeMeanStd(gyroX, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    computeMeanStd(gyroY, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    computeMeanStd(gyroZ, count, mean, stddev);
    fv.features[idx++] = mean;
    fv.features[idx++] = stddev;

    return fv;
}

void collectWindow(FeatureVector (&window)[WINDOW_SIZE], uint16_t *nSamples) {
	*nSamples = 0;
	while (*nSamples < WINDOW_SIZE) {
	    updateIMU(); 
	    if (bufferFilled && sampleIndex == 0) {
		window[(*nSamples)++] = computeFeatures();
	    }
	}
}

void updateEMA(FeatureVector (&windowBuffer)[WINDOW_SIZE], uint16_t nSamples) {
	for (size_t fi = 0; fi < NUM_FEATURES; fi++) {
		float mu = 0;
		for (size_t wi = 0; wi < nSamples; wi++) {
			mu += windowBuffer[wi].features[fi];
		}
		mu /= nSamples;
		EMAs[fi] = EMA_ALPHA * EMAs[fi] + (1 - EMA_ALPHA) * mu;
	}
}

void persistEMA(void) {
    	setKVPersistedEMA(EMAs);
}

void normalizeWindow(FeatureVector (&windowBuffer)[WINDOW_SIZE], uint16_t nSamples) {
	for (size_t wi = 0; wi < nSamples; wi++) {
		for (size_t fi = 0; fi < NUM_FEATURES; fi++) {
			windowBuffer[wi].features[fi] -= EMAs[fi];
		}
	}
}
