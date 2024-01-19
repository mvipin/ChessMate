#include <SoftwareSerial.h>
#include "Utils.h"

state_t state;
char legal_moves[LEGAL_MOVES_MAX][5];
uint8_t legal_moves_cnt;
char special_moves[MOVE_TYPE_MAX][5];

void setup() {
  serial_init();
  sensor_init();
  button_init();
  display_init();
  state = MOVE_NONE;

  Serial.begin(9600);
  Serial.println("Welcome to ChessMate!");
}

void loop() {
  scan_serial();
  scan_sensors();
  scan_buttons();
}
