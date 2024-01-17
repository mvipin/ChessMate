#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "Utils.h"

Adafruit_MCP23X17 mcp;

uint8_t sensor_pins[CHESS_ROWS][CHESS_COLS] = {
  {15, 14, 13, 12, 15, 14, 13, 12},
  {11, 10,  9,  8, 11, 10,  9,  8},
  { 7,  6,  5,  4,  7,  6,  5,  4},
  { 3,  2,  1,  0,  3,  2,  1,  0},
  {15, 14, 13, 12, 15, 14, 13, 12},
  {11, 10,  9,  8, 11, 10,  9,  8},
  { 7,  6,  5,  4,  7,  6,  5,  4},
  { 3,  2,  1,  0,  3,  2,  1,  0},
};

int8_t occupancy_init[CHESS_ROWS][CHESS_COLS];
int8_t occupancy_delta[CHESS_ROWS][CHESS_COLS];

bool validate_occupancy() {
  bool ret = true;
  static bool once_only = false;
  if (once_only) {
    return true;
  }

  reset_display();
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      // TODO: Remove me
      if ((i < 4) || (j > 3)) continue;

      if (!mcp.digitalRead(sensor_pins[i][j]) != occupancy_init[i][j]) {
        update_display(algebraic_notation[i][j], RED);
        //Serial.print(i);
        //Serial.print(",");
        //Serial.print(j);
        //Serial.print(": ");
        //Serial.print(sensor_pins[i][j]);
        //Serial.print(",");
        //Serial.print(occupancy_init[i][j]);
        //Serial.println();
        ret = false;
      }
    }
  }
  lightup_display();
  once_only = true;

  return ret;
}

void update_valid_moves(uint8_t row, uint8_t col) {
   char src[3];
   algebraic_lookup(row, col, src);
   update_display(src, MAGENTA);
   for (uint8_t i=0; i<legal_moves_cnt; i++) {
        // Check if the first two characters of the move match the starting square
        if (strncmp(legal_moves[i], src, 2) == 0) {
            char *dst = legal_moves[i] + 2;
            //Serial.println(dst);
            update_display(dst, GREEN);
        }
    }
}

void compute_delta() {
  reset_display();
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      // TODO: Remove me
      if ((i < 4) || (j > 3)) continue;

      occupancy_delta[i][j] = (!mcp.digitalRead(sensor_pins[i][j])) - occupancy_init[i][j];
      if (occupancy_delta[i][j] == -1) {
        // Calculate and highlight the possible moves for the piece lifted
        update_valid_moves(i, j);
      } else if (occupancy_delta[i][j] == 1) {
        // Calculate and validate the move
        char notation[3];
        algebraic_lookup(i, j, notation);
      }
    }
  }
  lightup_display();
}

void scan_sensors() {
  if ((state == MOVE_INIT) || (state == MOVE_NONE)) {
    return;
  }

  if (state == MOVE_START) {
    bool valid = validate_occupancy();
  }

  compute_delta();
}

void sensor_init() {
  if (!mcp.begin_I2C(0x27)) {
    Serial.println("Error.");
    return;
  }

  // configure pin for input with pull up
  for (uint8_t i=0; i<16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
}
