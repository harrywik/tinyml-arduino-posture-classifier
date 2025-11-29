#include "ble.h"
#include "esn.h"
#include "engine.h"
#include "button.h"
#include "imu_features.h"
#include "serial_protocol.h"

CommunicationMode coms;

void setup() {
    // Needed for the button interrupts
    __enable_irq();
    // Button init first as this decides com mode
    initButton();
    // Button interrupt handler
    onButtonPress(buttonHandler); // Important to have before

    // What is blow which is polling
    // Wait for button press
    coms = getCommunicationMode();
    if (coms == BLE)
      // Long press
      initBLE();
    else
      // Short press
      initSerial();

    // Sensor
    initIMU();
    // ML-model
    initESN();
}

void loop() {
  runIteration(coms);
}

