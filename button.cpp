#include "button.h"
#include "led.h"
#include <Arduino.h>
#include <Wire.h>

const long LONG_PRESS_THRESHOLD_MS = 5000; // 5 seconds (5 000 ms)
const long DEBOUNCE_MS = 100;              // Minimum time (100ms) that counts

void nrf_gpio_cfg_out_with_input(uint32_t pin_number) {
  nrf_gpio_cfg(pin_number, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_CONNECT,
               NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
}

// init button
void initButton(void) {
  pinMode(LARGE_BUTTON, OUTPUT);
  digitalWrite(LARGE_BUTTON, HIGH);
  nrf_gpio_cfg_out_with_input(digitalPinToPinName(LARGE_BUTTON));
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
    int buttonState = nrf_gpio_pin_read(digitalPinToPinName(LARGE_BUTTON));

    // START of a press: Button goes LOW and we haven't started tracking yet
    if (buttonState == LOW && !buttonPressed) {
      long currentTime = millis();

      // Initial check: if we've been running long enough,
      // the low reading might satisfy the long press, so we clear the button
      // state immediately if it's been HIGH for the full long-press time.

      // This is the core fix: A new press must reset the timer if it wasn't
      // already LOW.
      buttonPressed = true;
      pressStartTime = currentTime;
    }

    // CHECK for a LONG press: Still LOW after threshold
    if (buttonPressed && buttonState == LOW) {
      if (millis() - pressStartTime >= LONG_PRESS_THRESHOLD_MS) {
        // LONG PRESS DETECTED: BLE Mode (Blue LED)
        communicateBLEMode();
        return MODE_BLE;
      }
    }

    // END of a press: Button goes HIGH (released) AND we were tracking a press
    if (buttonState == HIGH && buttonPressed) {
      long pressDuration = millis() - pressStartTime;

      if (pressDuration >= DEBOUNCE_MS) {
        // SHORT PRESS DETECTED: USB Mode (Red LED)
        communicateUSBMode();
        return MODE_USB;
      }
      // Press was noise (too short). Reset state and time and restart the loop.
      buttonPressed = false;
      pressStartTime =
          0; // CRITICAL: Reset the timer to 0 to prevent the timeout
    }

    delay(5); // Stability
  }
}
