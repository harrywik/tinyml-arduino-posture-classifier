#pragma once
#include "esn.h"
#include <Arduino.h>
#include <stdint.h>

extern uint16_t CONFUSION_MATRIX[OUTPUT_SIZE][OUTPUT_SIZE];

void resetMetrics();

void printMultiClassMetrics();

void updateConfusionMatrix(const FeatureVector *testWindow,
                           const uint8_t *testLabels, uint16_t n_samples,
                           bool printMetrics = true);

void printResults();

void evaluateLoop();
