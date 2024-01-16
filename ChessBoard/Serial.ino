#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Utils.h"

#define UART_RX_PIN 3
#define UART_TX_PIN 2
#define MAX_TOKENS (4 * CHESS_ROWS + 2)

// Begin a software serial on pins D3 and D2 to talk to the Pi
SoftwareSerial mySerial (UART_RX_PIN, UART_TX_PIN);
char cmdstr[CMD_LEN_MAX];

int parse_command(char command[], char* tokens[]) {
    int token_count = 0;
    char* token = strtok(command, ":");

    while (token != NULL && token_count < MAX_TOKENS) {
        tokens[token_count] = token;
        token_count++;
        token = strtok(NULL, ":");
    }

    return token_count; // Return the number of tokens found
}

void process_cmd(char cmd[], int size) {
  // Extract the opcode and data
  char* tokens[MAX_TOKENS];
  int num_tokens = parse_command(cmd, tokens);

  // Process the opcode and data
  int idx = 0;
  if (strcmp(tokens[idx],"init") == 0) {
    state = MOVE_INIT;
    reset_display();
  } else if (strcmp(tokens[idx],"occupancy") == 0) {
    occupancy_init[CHESS_ROWS][CHESS_COLS] = {0};
    while (++idx < num_tokens) {
      int row = atoi(tokens[idx]) / CHESS_COLS;
      int col = atoi(tokens[idx]) % CHESS_ROWS;
      occupancy_init[row][col] = 1;
    }
    //print_matrix(occupancy_init);
  } else if (strcmp(tokens[idx],"legal") == 0) {
    while (++idx < num_tokens) {
      if (legal_moves_cnt >= LEGAL_MOVES_MAX) {
        Serial.println("Legal moves memory exhausted");
        return;
      }
      strncpy(legal_moves[legal_moves_cnt], tokens[idx], 4);
      legal_moves[legal_moves_cnt++][4] = '\0';
    }
  } else if (strcmp(tokens[idx],"start") == 0) {
    state = MOVE_START;
  } else if (strcmp(tokens[idx],"stop") == 0) {
    state = MOVE_STOP;
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
  
  if ((state == MOVE_INIT) || (state == MOVE_NONE)) {
    String cmd = check_for_cmd();
    if (cmd != NULL) {
      //Serial.println(cmd);
      cmd.toCharArray(cmdstr, CMD_LEN_MAX);
      process_cmd(cmdstr, CMD_LEN_MAX);
    }
  }
}

void serial_init() {
  mySerial.begin(9600); // Software serial for communicating with Pi
}
