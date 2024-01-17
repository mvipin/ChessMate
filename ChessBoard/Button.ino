#include "Utils.h"

#define PIN_BUTTON_CONFIRM 8
#define PIN_BUTTON_HINT 9
#define DEBOUNCE_DELAY 50

unsigned long lastDebounceTime[2] = {0, 0};  // the last time the output pin was toggled
uint8_t buttonState[2];             // the current reading from the input pin
uint8_t lastButtonState[2] = {LOW, LOW};   // the previous reading from the input pin
uint8_t btnpin[2] = {PIN_BUTTON_CONFIRM, PIN_BUTTON_HINT};
bool confirm;
bool hint;
   
void button_init() {
  pinMode(PIN_BUTTON_HINT, INPUT);
  pinMode(PIN_BUTTON_CONFIRM, INPUT);
}

void scan_buttons() {
  if ((state == MOVE_INIT) || (state == MOVE_NONE)) {
    return;
  }

  for (uint8_t i=0; i<2; i++) {
    // read the state of the switch into a local variable:
    int reading = digitalRead(btnpin[i]);

    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState[i]) {
      // reset the debouncing timer
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading != buttonState[i]) {
        buttonState[i] = reading;

        // only toggle the LED if the new button state is HIGH
        if (buttonState[i] == HIGH) {
          if (i == 0) {
            Serial.println("Confirm");
            confirm = true;
          } else {
            hint = true;
            Serial.println("Hint");
          }
        }
      }
    }
    
    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState[i] = reading;
  }
}
