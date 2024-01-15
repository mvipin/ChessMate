#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Utils.h"

const byte rxPin = 3; // Arduino RX = D3
const byte txPin = 2; // Arduino TX = D2

// Begin a software serial on pins D3 and D2 to talk to the Pi
SoftwareSerial mySerial (rxPin, txPin);

String check_for_cmd() {
  String input = mySerial.readStringUntil('\n');
  if (input != NULL) {
    input.trim();
  }
  return input;
}

void wait_for_pi_to_start() {
  int chess_squares_lit = 0;
  while(true) {
    chess_squares_lit = loading_status(chess_squares_lit);
    delay(100);
    // If the Pi has sent data, read the data buffer until no more data
    if (mySerial.available()) {
      while (mySerial.available()) {
        Serial.print(mySerial.read(), HEX);
      } // Read from Pi, print to hardware Serial
      Serial.println();
      break;
    }
  }
}

void serial_init() {
  mySerial.begin(9600); // Software serial for communicating with Pi
}
