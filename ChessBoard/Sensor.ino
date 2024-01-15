#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "Utils.h"

#define BUTTON_PIN 0  // MCP23XXX pin button is attached to

Adafruit_MCP23X17 mcp;

uint8_t sensor_matrix[4][4] = {
  {15, 14, 13, 12},
  {11, 10,  9,  8},
  { 7,  6,  5,  4},
  { 3,  2,  1,  0},
};

void get_occupancy_matrix(bool om[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      if (!mcp.digitalRead(sensor_matrix[i][j])) {
        om[i][j] = true;
      } else {
        om[i][j] = false;
      }
    }
  }
}

void print_occupancy_matrix(bool om[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      Serial.print(om[i][j]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

void save_occupancy_matrix(bool omo[4][4], bool omn[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      omo[i][j] = omn[i][j];
    }
  }
}

move_t detect_move(bool omo[4][4], bool omn[4][4], String &pos) {
  move_t move = MOVE_NONE;
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      if ((omn[i][j] - omo[i][j]) == 1) {
        notation_lookup(i, j, pos);
        move = MOVE_DOWN;
      } else if ((omn[i][j] - omo[i][j]) == -1) {
        move = MOVE_UP;
        notation_lookup(i, j, pos);
      }
    }
  }

  return move;
}

void sensor_init() {
  // uncomment appropriate mcp.begin
  if (!mcp.begin_I2C(0x27)) {
    Serial.println("Error.");
    while (1);
  }

  // configure pin for input with pull up
  for (int i=BUTTON_PIN; i<BUTTON_PIN+16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
}
