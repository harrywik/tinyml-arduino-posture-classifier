#include <Arduino.h>
#include "button.h"

const uint8_t USER_BUTTON_PIN = 4;
const long LONG_PRESS_THRESHOLD_MS = 3000; // 3 second (3000 ms)

// init button
void initButton(void) {
	// Use LED to communicate mode
	pinMode(LEDR, OUTPUT);
	pinMode(LEDG, OUTPUT);
	pinMode(LEDB, OUTPUT);

	// Turn off all colors initially (HIGH is OFF)
	digitalWrite(LEDR, HIGH); 
	digitalWrite(LEDG, HIGH);
	digitalWrite(LEDB, HIGH); 

	pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
}

/*
 *
 * Long press: BLE
 * Short press: USB
 * No press: function is called again by in PostureProject.setup()
 *
 * this is only executed on startup.
 *
 */

CommunicationMode getCommunicationMode(void) {

	// Timing and state variables
	long pressStartTime = 0;
	bool buttonPressed = false;
	int buttonState = digitalRead(USER_BUTTON_PIN);

  	// If the button is LOW (pressed) AND we haven't started tracking a press yet
  	if (buttonState == LOW && !buttonPressed) {
    		// Mark the button as pressed
    		buttonPressed = true;
    		// Record the start time (Debouncing handled by only setting time once)
    		pressStartTime = millis();
  	}

  	// If the button is still pressed AND the long press time has passed
  	if (buttonPressed && buttonState == LOW && millis() - pressStartTime >= LONG_PRESS_THRESHOLD_MS) {
		communicateBLEMode();
		return BLE;
	}

	// If the button is HIGH (released) AND we were tracking a press
	if (buttonState == HIGH && buttonPressed) {
		long pressDuration = millis() - pressStartTime;
		// Check if the press lasted long enough to be intentional
		if (pressDuration > 50) {
			communicateUSBMode();
			return USB;
		}
      	} 

	// Add small delay before checking again
  	delay(5);
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
