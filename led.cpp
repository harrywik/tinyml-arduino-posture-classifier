#include "led.h"
#include <Arduino.h>

void turnOffLED(void) {
	// Turn off all colors (HIGH is OFF)
	digitalWrite(LEDR, HIGH); 
	digitalWrite(LEDG, HIGH);
	digitalWrite(LEDB, HIGH); 
}

void initLED(void) {
	// Use LED to communicate mode
	pinMode(LEDR, OUTPUT);
	pinMode(LEDG, OUTPUT);
	pinMode(LEDB, OUTPUT);
	turnOffLED();
}

void communicateUSBMode(void) {
	// Red is USB
	// LOW is ON
	digitalWrite(LEDR, LOW);
}

void communicateBLEMode(void) {
	// Blue is BLE
	// LOW is ON
	digitalWrite(LEDB, LOW);
}

void communicatePersistance(void) {
	turnOffLED();
	// Green is persistance
	// LOW is ON
	digitalWrite(LEDG, LOW);
}
