#include "esn.h"
#include "engine.h"
#include "imu_features.h"
#include "serial_protocol.h"

void setup() {
    initSerial();
    initIMU();
    initESN();
}

void loop() {
  runIteration();
}

