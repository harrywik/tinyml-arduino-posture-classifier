#include "ble.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "imu_features.h"
#include "serial_protocol.h"

CommunicationMode coms;

void setup() {
    initButton();

    // Wait for button press
    coms = getCommunicationMode();
    if (coms == BLE)
      // Long press
      initBLE();
    else
      // Short press
      initSerial();

    initIMU();
    initESN();
}

void loop() {
  runIteration(coms);
}

