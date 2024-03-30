#include "Utils.h"

move_state_t state;
char legal_moves[LEGAL_MOVES_MAX][5];
uint8_t legal_moves_cnt;
char check_squares[CHECK_SQUARES_MAX][3];
uint8_t check_squares_cnt;
char special_moves[MOVE_TYPE_MAX][5];
bool confirm, hint;
uint8_t hint_override_cnt = 0;
unsigned long prev_override_ms = 0;
uint8_t confirm_zreset_cnt = 0;
unsigned long prev_zreset_ms = 0;

void process_board() {
  if (state == MOVE_RESET) {
    confirm = false;
    hint = false;
  } else if (state == MOVE_START) {
    // Need clue?
    if (hint) {
      hint = false;
      highlight_move(special_moves[MOVE_TYPE_HINT], MAGENTA, MAGENTA);
      delay(500);

      // Detect user override
      unsigned long curr_override_ms = millis();
      if (hint_override_cnt == 0 || curr_override_ms - prev_override_ms <= HINT_OVERRIDE_INTERVAL) {
        hint_override_cnt++;
        prev_override_ms = curr_override_ms;  // Reset the timer
        if (hint_override_cnt == HINT_OVERRIDE_CNT) {
          state = MOVE_STOP;
          hint_override_cnt = 0;
          legal_moves_cnt = 0;
          reset_occupancy();
          send_response("ffff");
        }
      } else {
        hint_override_cnt = 1;
        prev_override_ms = curr_override_ms;
      }
    }

    // Display if the user is in check
    if (check_squares_cnt) {
      reset_display();
      uint8_t row, col;
      xy_lookup(check_squares[0], row, col);
      update_display(row, col, ORANGE);
      for (int i=1; i<check_squares_cnt; i++) {
        xy_lookup(check_squares[i], row, col);
        update_display(row, col, GREEN);
      }
      lightup_display();
      delay(5000);
      check_squares_cnt = 0;
    }
  }
  
  // Move finalized?
  if (confirm) {
    confirm = false;
    if (state == MOVE_START) {
      char move[5];
      if (compute_move(move)) {
        send_response(move);
        state = MOVE_STOP;
        legal_moves_cnt = 0;
        check_squares_cnt = 0;
        reset_occupancy();
        set_control_pixel(HUMAN, BLACK);
        set_control_pixel(COMPUTER, GREEN);
        send_indication("i");
      } else {
        state = MOVE_RESET;
        unsigned long curr_zreset_ms = millis();
        if (confirm_zreset_cnt == 0 || curr_zreset_ms - prev_zreset_ms <= CONFIRM_ZRESET_INTERVAL) {
          confirm_zreset_cnt++;
          prev_zreset_ms = curr_zreset_ms;  // Reset the timer
          if (confirm_zreset_cnt == CONFIRM_ZRESET_CNT) {
            send_indication("j");
            confirm_zreset_cnt = 0;
          }
        } else {
          confirm_zreset_cnt = 1;
          prev_zreset_ms = curr_zreset_ms;
        }
      }
    } else if (state == MOVE_COMP) {
      // TODO: For now, we will just send the ACK assuming the player 
      // moving the pieces on behalf of the computer is doing the right 
      // thing. Eventually, the move should be generated by sensing the board
      send_response(special_moves[MOVE_TYPE_COMP]);
      state = MOVE_STOP;
    } else if (state == MOVE_OVERRIDE) {
      send_response(special_moves[MOVE_TYPE_OVERRIDE]);
      state = MOVE_STOP;
      set_control_pixel(HUMAN, BLACK);
      set_control_pixel(COMPUTER, GREEN);
    } else if (state == MOVE_CHECKMATE) {
      send_indication("z");
      display_win(special_moves[MOVE_TYPE_CHECKMATE]);
      state = MOVE_INIT;
      reset_display();
      lightup_display();
    }
  }
}

void debug_init() {
  Serial.begin(9600);
  for (uint16_t trial = 0; trial < 2000; trial++) {
      if (Serial) {
        Serial.print("Debug initialized: ");
        Serial.println(trial);
        break;
      }
  }
}

void setup() {
  debug_init();
  serial_init();
  display_init();
  sensor_init();
  button_init();
  state = MOVE_NONE;
  Serial.println("Welcome to ChessMate!");
}

void loop() {
  scan_serial();
  scan_sensors();
  scan_buttons();
  process_board();
}
