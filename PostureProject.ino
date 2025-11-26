#include "main.h"
#include "esn.h"
#include "imu_features.h"
#include "serial_protocol.h"

void setup() {
    initSerial();
    initIMU();
    initESN();
}

void loop() {
  main();
}

