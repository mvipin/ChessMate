#include "Utils.h"

#define PIN_BUTTON_CONFIRM 8
#define PIN_BUTTON_HINT 9

unsigned long lastDebounceTime[] = {0, 0};  // the last time the output pin was toggled
const unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonState[2];             // the current reading from the input pin
int lastButtonState[] = {LOW, LOW};   // the previous reading from the input pin
uint8_t btnpin[] = {PIN_BUTTON_CONFIRM, PIN_BUTTON_HINT};
 
void button_init() {
  pinMode(PIN_BUTTON_HINT, INPUT);
  pinMode(PIN_BUTTON_CONFIRM, INPUT);
}

void scan_buttons(bool &confirm, bool &hint) {
  confirm = false;
  hint = false;
  for (int i=0; i<2; i++) {
    // read the state of the switch into a local variable:
    int reading = digitalRead(btnpin[i]);

    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState[i]) {
      // reset the debouncing timer
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading != buttonState[i]) {
        buttonState[i] = reading;

        // only toggle the LED if the new button state is HIGH
        if (buttonState[i] == HIGH) {
          if (i == 0) {
            confirm = true;
          } else {
            hint = true;
          }
        }
      }
    }
    
    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState[i] = reading;
  }
}
