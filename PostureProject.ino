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
    Coms.begin();

    Coms.setBackend(coms);
    // if (coms == BLE)
    //   // Long press
    //   initBLE();
    // else
    //   // Short press
    //   initSerial();

    // Sensor
    initIMU();
    // ML-model
    initESN();

    // Button interrupt handler
    onButtonPress(buttonHandler);
}

void loop() {
  runIteration();
}

