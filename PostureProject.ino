#include "imu_features.h"
#include "esn.h"
#include "serial_protocol.h"

FeatureVector windowBuffer[WINDOW_SIZE];
uint8_t labelsBuffer[WINDOW_SIZE];
uint16_t nSamples = 0;

void setup() {
    initSerial();
    initIMU();
    initESN();
}

void loop() {
    updateIMU();
    FeatureVector fv = computeFeatures();

    updateReservoir(fv);        // Update ESN state

    SerialCommand cmd = readSerialCommand();
    switch (cmd.type) {
        case CMD_LABEL:
            // Save labeled feature for training
            windowBuffer[nSamples] = fv;
            labelsBuffer[nSamples] = cmd.label;
            nSamples++;
            sendMessage("Sample stored");
            break;

        case CMD_TRAIN:
            trainOutputLayer(windowBuffer, labelsBuffer, nSamples, 0.01f);
            sendMessage("Training completed");
            nSamples = 0;
            break;

        case CMD_INFER:
            uint8_t pred = predict();
            sendPrediction(pred);
            break;

        case CMD_STOP:
            sendMessage("Stopping loop");
            break;

        default:
            break;
    }
}

