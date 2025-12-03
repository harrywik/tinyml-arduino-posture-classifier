#include <stdint.h>
#include <Arduino.h>

extern uint16_t CONFUSION_MATRIX[3][3];

void resetMetrics();

void printMultiClassMetrics(uint16_t n_samples);

void evaluateModel(const FeatureVector* testWindow, const uint8_t* testLabels,\
     uint16_t n_samples, bool printMetrics=true);
