/*
 * USER_LOGIC_SOLARNODE.CPP - Implementation for Solar Node specific logic
 */

#include <Arduino.h>
#include "user_logic_solarnode.h"

namespace user_logic {

// Debounce logic for the user button
const unsigned long LONG_PRESS_DURATION = 2000; // 2 seconds for a long press
unsigned long button_press_start_time = 0;
bool button_is_pressed = false;

void begin() {
    // CRITICAL: Set POWER_CTRL_PIN to HIGH immediately to keep the board powered on.
    // This pin controls the power latch on the PMIC.
    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, HIGH);

    // Configure the user button with an internal pull-up resistor.
    pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
}

void handle() {
    // Read the state of the user button (it's pulled up, so LOW means pressed)
    bool current_button_state = (digitalRead(USER_BUTTON_PIN) == LOW);

    if (current_button_state && !button_is_pressed) {
        // Button was just pressed
        button_press_start_time = millis();
        button_is_pressed = true;
    } else if (!current_button_state && button_is_pressed) {
        // Button was just released
        button_is_pressed = false;
        button_press_start_time = 0;
    }

    // Check for a long press while the button is held down
    if (button_is_pressed && (millis() - button_press_start_time > LONG_PRESS_DURATION)) {
        // Long press detected, initiate power off sequence.
        powerOff();
    }
}

void powerOff() {
    // To turn off the board, we set the power control pin to LOW.
    // This will release the power latch in the PMIC.
    digitalWrite(POWER_CTRL_PIN, LOW);
    
    // The board will lose power after this, so no further code will execute.
    // We can add a small delay to ensure the command is processed.
    delay(100); 
}

} // namespace user_logic
