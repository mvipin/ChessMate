#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Utils.h"

#define UART_RX_PIN 3
#define UART_TX_PIN 2
#define MAX_TOKENS (4 * CHESS_ROWS + 2)

// Begin a software serial on pins D3 and D2 to talk to the Pi
SoftwareSerial mySerial (UART_RX_PIN, UART_TX_PIN);
char cmdstr[CMD_LEN_MAX];

void print_legal_moves() {
  for (int i=0; i<legal_moves_cnt; i++) {
    Serial.print(legal_moves[i]);
    Serial.print(" ");
  }
  Serial.println();
}

int parse_command(char command[], char* tokens[]) {
    uint8_t token_count = 0;
    char* token = strtok(command, ":");

    while (token != NULL && token_count < MAX_TOKENS) {
        tokens[token_count] = token;
        token_count++;
        token = strtok(NULL, ":");
    }

    return token_count; // Return the number of tokens found
}

void process_cmd(char cmd[], uint8_t size) {
  // Extract the opcode and data
  char* tokens[MAX_TOKENS];
  uint8_t num_tokens = parse_command(cmd, tokens);

  // Process the opcode and data
  uint8_t idx = 0;
  if (strcmp(tokens[idx],"init") == 0) {
    state = MOVE_INIT;
    reset_display();
    lightup_display();
  } else if (strcmp(tokens[idx],"occupancy") == 0) {
    reset_occupancy();
    while (++idx < num_tokens) {
      uint8_t row = atoi(tokens[idx]) / CHESS_COLS;
      // Hackish since the host sends the data starting from 'a1' instead of 'a8'
      row = 7 - row;
      uint8_t col = atoi(tokens[idx]) % CHESS_ROWS;
      occupancy_init[row][col] = 1;
    }
    //print_matrix(occupancy_init);
  } else if (strcmp(tokens[idx],"legal") == 0) {
    while (++idx < num_tokens) {
      if (legal_moves_cnt >= LEGAL_MOVES_MAX) {
        Serial.println("Legal moves memory exhausted");
        display_fatal_error();
        return;
      }
      strncpy(legal_moves[legal_moves_cnt], tokens[idx], 4);
      legal_moves[legal_moves_cnt++][4] = '\0';
    }
  } else if (strcmp(tokens[idx],"hint") == 0) {
    strncpy(special_moves[MOVE_TYPE_HINT], tokens[++idx], 4);
    special_moves[MOVE_TYPE_HINT][4] = '\0';
  } else if (strcmp(tokens[idx],"start") == 0) {
    set_control_pixel(HUMAN, GREEN);
    set_control_pixel(COMPUTER, BLACK);
    print_legal_moves();
    state = MOVE_RESET;
  } else if (strcmp(tokens[idx],"fish") == 0) {
    highlight_move(tokens[++idx]);
    strncpy(special_moves[MOVE_TYPE_COMP], tokens[idx], 4);
    special_moves[MOVE_TYPE_COMP][4] = '\0';
    Serial.print("comp: ");
    Serial.println(special_moves[MOVE_TYPE_COMP]);
    state = MOVE_COMP;
  }
}

String check_for_cmd() {
  String input = mySerial.readStringUntil('\n');
  if (input != NULL) {
    input.trim();
  }
  return input;
}

void scan_serial() {
  if (state == MOVE_NONE) {
    static int chess_squares_lit = 0;
    chess_squares_lit = loading_status(chess_squares_lit);
  }
  
  if ((state == MOVE_INIT) || (state == MOVE_NONE) || (state == MOVE_STOP)) {
    String cmd = check_for_cmd();
    if (cmd != NULL) {
      cmd.toCharArray(cmdstr, CMD_LEN_MAX);
      process_cmd(cmdstr, CMD_LEN_MAX);
    }
  }
}

void send_response(char resp[]) {
  mySerial.println(resp);
}

void serial_init() {
  mySerial.begin(9600); // Software serial for communicating with Pi
}
