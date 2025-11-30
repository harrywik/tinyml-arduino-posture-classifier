#include "ble.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "imu_features.h"
#include "serial_protocol.h"
#include "io.h"

CommunicationMode coms;

extern IO Coms;

void setup() {
    initButton();
    // Wait for button press
    coms = getCommunicationMode();
    // This handles initialization
    // of the correct IO device
    Coms.setBackend(coms);
    // Sensor
    initIMU();
    // ML-model
    initESN();
}

void loop() {
  runIteration();
}

