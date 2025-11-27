#include "esn.h"
#include "engine.h"
#include "button.h"
#include "imu_features.h"
#include "serial_protocol.h"

CommunicatonMode coms = NOT_INITIATED;

void setup() {
    while (coms == NOT_INITIATED)
      // No press
      coms = getCommunicationMode();
    
    if (coms == USB)
      // Short press
      initSerial();
    else 
      // Long press
      initBLE();

    initIMU();
    initESN();
}

void loop() {
  runIteration(coms);
}

