#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "Utils.h"

#define MOVEMENT_TYPE_ABSENT (1<<0) // instantaneous state
#define MOVEMENT_TYPE_PRESENT (1<<1) // instantaneous state
#define MOVEMENT_TYPE_REMOVE (1<<2) // sticky bit, persistent for a single move
#define MOVEMENT_TYPE_ADD (1<<3) // sticky bit, persistent for a single move

Adafruit_MCP23X17 mcp;

const uint8_t sensor_pins[CHESS_ROWS][CHESS_COLS] PROGMEM = {
  {15, 14, 13, 12, 15, 14, 13, 12},
  {11, 10,  9,  8, 11, 10,  9,  8},
  { 7,  6,  5,  4,  7,  6,  5,  4},
  { 3,  2,  1,  0,  3,  2,  1,  0},
  {15, 14, 13, 12, 15, 14, 13, 12},
  {11, 10,  9,  8, 11, 10,  9,  8},
  { 7,  6,  5,  4,  7,  6,  5,  4},
  { 3,  2,  1,  0,  3,  2,  1,  0},
};

bool occupancy_init[CHESS_ROWS][CHESS_COLS];
uint8_t occupancy_delta[CHESS_ROWS][CHESS_COLS];

void reset_occupancy() {
  for (int i=0; i<CHESS_ROWS; i++) {
    for (int j=0; j<CHESS_COLS; j++) {
      occupancy_init[i][j] = false;
      occupancy_delta[i][j] = 0;
    }
  }
}

bool validate_occupancy() {
  bool ret = true;

  reset_display();
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      // TODO: Remove me
      if ((i < 4) || (j > 3)) continue;
      bool present = !mcp.digitalRead(pgm_read_byte_near(&sensor_pins[i][j]));
      bool occupied = occupancy_init[i][j];
      if ((present && !occupied) || (!present && occupied)) {
        update_display(i, j, RED);
        ret = false;
      }
    }
  }
  lightup_display();

  return ret;
}

void show_valid_moves() {
  reset_display();
  // Highlight the possible moves for the piece(s) lifted
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      if ((i < 4) || (j > 3)) continue; // TODO: Remove me
      if (!(occupancy_delta[i][j] & MOVEMENT_TYPE_ABSENT)) continue;

      char src[3];
      get_algebraic_notation(i, j, src);
      update_display(i, j, YELLOW);
      for (uint8_t k=0; k<legal_moves_cnt; k++) {
        // Check if the first two characters of the move match the starting square
        if (strncmp(legal_moves[k], src, 2) == 0) {
          char *dst = legal_moves[k] + 2;
          uint8_t m, n;
          xy_lookup(dst, m, n);
          update_display(m, n, GREEN);
        }
      }
    }
  }
  lightup_display();
}

board_state_t occupancy_changed() {
  int8_t sum = 0;
  uint8_t changes = 0;
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      if ((i < 4) || (j > 3)) continue; // TODO: Remove me
      if (occupancy_delta[i][j] & MOVEMENT_TYPE_PRESENT) {
        sum += 1;
        changes++;
      } else if (occupancy_delta[i][j] & MOVEMENT_TYPE_ABSENT) {
        sum -= 1;
        changes++;
      }
    }
  }

  if ((sum > 0) || (sum < -1)) {
    display_fatal_error();
    delay(5000); // TODO: Reboot the platform
  }
  
  if (!changes) {
    return BOARD_STATE_NONE_MOVED;
  } else if (!sum) {
    return BOARD_STATE_PIECE_MOVED;
  }

  return BOARD_STATE_PIECE_REMOVED;
}

bool calculate_move_with_piece_moved(char move[]) {
  char dst[3];
  bool dst_found = false;
  // check for a valid destination
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      if ((i < 4) || (j > 3)) continue; // TODO: Remove me
      if (!(occupancy_delta[i][j] & MOVEMENT_TYPE_PRESENT)) continue;
      
      get_algebraic_notation(i, j, dst);
      for (uint8_t k=0; k<legal_moves_cnt; k++) {
        // Check if the last two characters of the move match the starting square
        if (strncmp(legal_moves[k]+2, dst, 2) == 0) {
          update_display(i, j, ORANGE);
          dst_found = true;
          goto src;
        }
      }
    }
  }
  if (!dst_found) return false;

src:
  // check for a valid source
  char src[3];
  bool src_found = false;
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      if ((i < 4) || (j > 3)) continue; // TODO: Remove me
      if (!(occupancy_delta[i][j] & MOVEMENT_TYPE_ABSENT)) continue;
      
        get_algebraic_notation(i, j, src);
        for (uint8_t k=0; k<legal_moves_cnt; k++) {
          // Check if the last two characters of the move match the starting square
          if ((strncmp(legal_moves[k], src, 2) == 0) && (strncmp(legal_moves[k]+2, dst, 2) == 0)) {
            if (src_found) {
              Serial.print("Mul src: ");
              Serial.println(src);
              return false; // Multiple sources
            }
            update_display(i, j, CYAN);
            src_found = true;
          }
        }

    }
  }

  if (src_found) {
    strncpy(move, src, 2);
    strncpy(move+2, dst, 2);
    move[4] = '\0';
    return true;
  }

  return false;
}

bool calculate_move_with_piece_removed(char move[]) {
  Serial.println("ERROR");
  print_matrix(occupancy_delta);
  return false;
}

bool compute_move(char move[]) {
  bool status = false;
  
  reset_display();
  board_state_t change = occupancy_changed();
  if (change == BOARD_STATE_NONE_MOVED) {
    return status;
  } else if (change == BOARD_STATE_PIECE_MOVED) {
    status = calculate_move_with_piece_moved(move);
  } else {
    status = calculate_move_with_piece_removed(move);
  }
  lightup_display();
  
  return status;
}

void compute_delta() {
  for (uint8_t i=0; i<CHESS_ROWS; i++) {
    for (uint8_t j=0; j<CHESS_COLS; j++) {
      if ((i < 4) || (j > 3)) continue; // TODO: Remove me
      bool present = !mcp.digitalRead(pgm_read_byte_near(&sensor_pins[i][j]));
      bool occupied = occupancy_init[i][j];
      occupancy_delta[i][j] &= ~(MOVEMENT_TYPE_ABSENT | MOVEMENT_TYPE_PRESENT);
      if (occupied && !present) {
        occupancy_delta[i][j] |= MOVEMENT_TYPE_REMOVE;
        occupancy_delta[i][j] |= MOVEMENT_TYPE_ABSENT;
      } else if (!occupied && present) {
        occupancy_delta[i][j] |= MOVEMENT_TYPE_ADD;
        occupancy_delta[i][j] |= MOVEMENT_TYPE_PRESENT;
      }
      if ((occupancy_delta[i][j] & MOVEMENT_TYPE_REMOVE) && present) {
        occupancy_delta[i][j] |= MOVEMENT_TYPE_ADD;
      }
      if ((occupancy_delta[i][j] & MOVEMENT_TYPE_ADD) && !present) {
        occupancy_delta[i][j] |= MOVEMENT_TYPE_REMOVE;
      }
    }
  }
}

void scan_sensors() {
  if (state == MOVE_RESET) {
    while (!validate_occupancy());
    state = MOVE_START;
  }
  
  if (state == MOVE_START) {
    compute_delta();
    show_valid_moves();
  }
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
