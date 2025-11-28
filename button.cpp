#include <Arduino.h>
#include "button.h"

const long LONG_PRESS_THRESHOLD_MS = 10000; // 10 seconds (10 000 ms)
const long DEBOUNCE_MS = 500; // Minimum time (500ms) that counts
const uint8_t LARGE_BUTTON = 4;

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

	pinMode(LARGE_BUTTON, INPUT_PULLUP);
}

/*
 *
 * Long press: BLE
 * Short press: USB
 *
 * this is only executed on startup.
 *
 */

CommunicationMode getCommunicationMode(void) {
    long pressStartTime = 0;
    bool buttonPressed = false;
    
    // Outer loop repeats until a valid mode is returned
    while (true) { 
        int buttonState = digitalRead(LARGE_BUTTON);

        // START of a press: Button goes LOW and we haven't started tracking yet
        if (buttonState == LOW && !buttonPressed) {
            long currentTime = millis();
            
            // Initial check: if we've been running long enough, 
            // the low reading might satisfy the long press, so we clear the button state 
            // immediately if it's been HIGH for the full long-press time.
            
            // This is the core fix: A new press must reset the timer if it wasn't already LOW.
            buttonPressed = true;
            pressStartTime = currentTime;
        }

        // CHECK for a LONG press: Still LOW after threshold
        if (buttonPressed && buttonState == LOW) {
            if (millis() - pressStartTime >= LONG_PRESS_THRESHOLD_MS) {
                // LONG PRESS DETECTED: BLE Mode (Blue LED)
                communicateBLEMode(); 
                return BLE; 
            }
        }
        
        // END of a press: Button goes HIGH (released) AND we were tracking a press
        if (buttonState == HIGH && buttonPressed) {
            long pressDuration = millis() - pressStartTime;

            if (pressDuration >= DEBOUNCE_MS) { 
                // SHORT PRESS DETECTED: USB Mode (Red LED)
                communicateUSBMode(); 
                return USB; 
            }
            // Press was noise (too short). Reset state and time and restart the loop.
            buttonPressed = false; 
            pressStartTime = 0; // CRITICAL: Reset the timer to 0 to prevent the timeout
        }

        delay(5); // Stability
    }
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
