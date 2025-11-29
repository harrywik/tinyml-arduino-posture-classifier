#include <Arduino.h>
#include <Wire.h>
#include "button.h"

#define BUTTON_PIN_NUMBER digitalPinToPinName(LARGE_BUTTON)
#define NRF_GPIO_SENSE_REG(pin) (*((volatile uint32_t *)((uint32_t)NRF_P0 + 0x510 + (pin) * 4)))

const long LONG_PRESS_THRESHOLD_MS = 10000;
const long DEBOUNCE_MS = 100;

static CallbackFunction button_press_callback = NULL;

void nrf_gpio_cfg_out_with_input(uint32_t pin_number) {
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_SENSE_LOW // SENSE_LOW for falling edge detection
    );
}

// Note: We use the system's provided macros/definitions like GPIO_PIN_CNF_SENSE_Msk
void GPIOTE_IRQHandler(void) {
    // Check and Clear the PORT event flag FIRST
    if (NRF_GPIOTE->EVENTS_PORT != 0) {
        // Clear the event flag by writing 0
        NRF_GPIOTE->EVENTS_PORT = 0;

        // Check the current state of the button pin
        // Check if the pin is currently LOW (i.e., the press is still active/valid)
        if (!(NRF_GPIO->IN & (1UL << BUTTON_PIN_NUMBER))) { 
            // Pin is LOW (Button Pressed)
            if (button_press_callback != NULL) {
                // Execute the user's callback function
                button_press_callback();
            }
        }
        uint32_t pin_cnf = NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER];
        // Temporarily clear SENSE (Write 0 to the SENSE field)
        NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk; 
        // Re-set SENSE to LOW
        NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = (pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk) | 
                                               (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos); 
    }
}

// --- 3. initButton (Cleaned up SENSE setup) ---
void initButton(void) {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);

    // Keep the working OUTPUT configuration
    pinMode(LARGE_BUTTON, OUTPUT);
    digitalWrite(LARGE_BUTTON, HIGH);
    nrf_gpio_cfg_out_with_input(digitalPinToPinName(LARGE_BUTTON));

    // Enable GPIOTE interrupt
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);
}

void onButtonPress(CallbackFunction cb) {
    button_press_callback = cb;
}

// buttonPressIgnore is a long-polling utility that seems unrelated to the fix.
// It is left unchanged for now.
bool buttonPressIgnore(void) {
    // ... (Code omitted for brevity) ...
    size_t buttonState;
    long start = millis();
    while (true) {
        buttonState = nrf_gpio_pin_read(digitalPinToPinName(LARGE_BUTTON));
        if (buttonState == LOW) {
            if ((millis() - start) > 3000) {
                return false;
            }
        } else {
            return true;
        }
    }
}

CommunicationMode getCommunicationMode(void) {
    long pressStartTime = 0;
    bool buttonPressed = false;

    uint32_t pin_cnf = NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER];

    while (true) {
        int buttonState = nrf_gpio_pin_read(digitalPinToPinName(LARGE_BUTTON));

        // START of a press
        if (buttonState == LOW && !buttonPressed) {
            buttonPressed = true;
            pressStartTime = millis();
        }

        // LONG press check (Still LOW after threshold)
        if (buttonPressed && buttonState == LOW) {
            if (millis() - pressStartTime >= LONG_PRESS_THRESHOLD_MS) {
                communicateBLEMode();
                // Manually clear the event and re-arm SENSE so the interrupt works later.
                NRF_GPIOTE->EVENTS_PORT = 0;
                NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk; 
                NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = (pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk) | 
                                                       (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
                return BLE;
            }
        }

        // END of a press (Short press check)
        if (buttonState == HIGH && buttonPressed) {
            long pressDuration = millis() - pressStartTime;

            if (pressDuration >= DEBOUNCE_MS) {
                communicateUSBMode();
                
                // --- CRITICAL INTERRUPT CLEANUP ---
                // Manually clear the event and re-arm SENSE so the interrupt works later.
                NRF_GPIOTE->EVENTS_PORT = 0;
                NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk; 
                NRF_GPIO->PIN_CNF[BUTTON_PIN_NUMBER] = (pin_cnf & ~GPIO_PIN_CNF_SENSE_Msk) | 
                                                       (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
                // ------------------------------------

                return USB;
            }
            // Press was noise. Reset.
            buttonPressed = false;
            pressStartTime = 0;
        }

        delay(5);
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

void communicatePersistance(void) {
	// Turn off all colors (HIGH is OFF)
	digitalWrite(LEDR, HIGH); 
	digitalWrite(LEDG, HIGH);
	digitalWrite(LEDB, HIGH); 

	// Communicate it is now safe to unplug
	// Weights are shared
	digitalWrite(LEDG, LOW);
}
