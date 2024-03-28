#include "Utils.h"

#define UART_RX_PIN 9
#define UART_TX_PIN 8
#define MAX_TOKENS (4 * CHESS_ROWS + 2)

UART Serial2(UART_TX_PIN, UART_RX_PIN, NC, NC);

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
    Serial.println("Host initialized");
    reset_display();
    lightup_display();
  } else if (strcmp(tokens[idx],"occupancy") == 0) {
    reset_occupancy();
    while (++idx < num_tokens) {
      uint8_t row = atoi(tokens[idx]) / CHESS_COLS;
      uint8_t col = atoi(tokens[idx]) % CHESS_ROWS;
      occupancy_init[row][col] = true;
    }
  } else if (strcmp(tokens[idx],"legal") == 0) {
    while (++idx < num_tokens) {
      if (legal_moves_cnt >= LEGAL_MOVES_MAX) {
        Serial.println("no mem");
        display_fatal_error();
        delay(5000); // TODO: Reboot the platform
        return;
      }
      strncpy(legal_moves[legal_moves_cnt], tokens[idx], 4);
      legal_moves[legal_moves_cnt++][4] = '\0';
    }
  } else if (strcmp(tokens[idx],"hint") == 0) {
    strncpy(special_moves[MOVE_TYPE_HINT], tokens[++idx], 4);
    special_moves[MOVE_TYPE_HINT][4] = '\0';
  } else if (strcmp(tokens[idx],"check") == 0) {
    while (++idx < num_tokens) {
      if (check_squares_cnt >= CHECK_SQUARES_MAX) {
        Serial.println("no mem sq");
        display_fatal_error();
        delay(5000); // TODO: Reboot the platform
        return;
      }
      strncpy(check_squares[check_squares_cnt], tokens[idx], 2);
      check_squares[check_squares_cnt++][3] = '\0';
    }
  } else if (strcmp(tokens[idx],"start") == 0) {
    set_control_pixel(HUMAN, GREEN);
    set_control_pixel(COMPUTER, BLACK);
    send_indication("s");
    print_legal_moves();
    state = MOVE_RESET;
  } else if (strcmp(tokens[idx],"override") == 0) {
    strncpy(special_moves[MOVE_TYPE_OVERRIDE], tokens[++idx], 4);
    special_moves[MOVE_TYPE_OVERRIDE][4] = '\0';
    while (!validate_occupancy());
    uint8_t row, col;
    xy_lookup(special_moves[MOVE_TYPE_OVERRIDE]+2, row, col);
    uint16_t color = GREEN;
    if (occupancy_init[row][col]) {
      color = ORANGE;
    }
    highlight_move(special_moves[MOVE_TYPE_OVERRIDE], GREEN, color);
    state = MOVE_OVERRIDE;
    Serial.print("override: ");
    Serial.println(special_moves[MOVE_TYPE_OVERRIDE]);
  } else if (strcmp(tokens[idx],"comp") == 0) {
    strncpy(special_moves[MOVE_TYPE_COMP], tokens[++idx], 4);
    special_moves[MOVE_TYPE_COMP][4] = '\0';
    while (!validate_occupancy());
    uint8_t row, col;
    xy_lookup(special_moves[MOVE_TYPE_COMP]+2, row, col);
    uint16_t color = GREEN;
    if (occupancy_init[row][col]) {
      color = ORANGE;
    }
    highlight_move(special_moves[MOVE_TYPE_COMP], GREEN, color);
    Serial1.print(tokens[idx]);
    state = MOVE_COMP;
    Serial.print("comp: ");
    Serial.println(special_moves[MOVE_TYPE_COMP]);
    while (true) {
      String ack = Serial1.readStringUntil('\n');
      if (ack != NULL) {
        ack.trim();
        Serial.print("Arm Ack: ");
        Serial.println(ack);
        if (ack == "done") {
          confirm = true;
          break;
        }
      }
    }
  } else if (strcmp(tokens[idx],"checkmate") == 0) {
    char dst[3];
    strncpy(dst, tokens[++idx], 2);
    dst[2] = '\0';
    strncpy(special_moves[MOVE_TYPE_CHECKMATE], tokens[++idx], 4);
    special_moves[MOVE_TYPE_CHECKMATE][4] = '\0';
    state = MOVE_CHECKMATE;
    reset_display();
    uint8_t row, col;
    xy_lookup(dst, row, col);
    for (int i=-1; i<=1; i++) {
      for (int j=-1; j<=1; j++) {
        if ((i == 0) && (j == 0)) continue; // king square itself
        if ((row == 0) && (i == -1)) continue; // skip negative row index
        if ((col == 0) && (j == -1)) continue; // skip negative col index
        if ((row == 7) && (i == 1)) continue; // skip out-of-bounds row index
        if ((col == 7) && (j == 1)) continue; // skip out-of-bounds col index
        if (!occupancy_init[row+i][col+j]) {
          update_display(row+i, col+j, ORANGE);
        }
      }
    }
    lightup_display();
  } else if (strcmp(tokens[idx],"reset") == 0) {
    state = MOVE_INIT;
    display_count_up();
    reset_display();
    lightup_display();
  }
}

String check_for_cmd() {
  String input = Serial2.readStringUntil('\n');
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

// To the host
void send_response(char resp[]) {
  Serial2.println(resp);
}

// To the robotic arm
void send_indication(char ind[]) {
  Serial1.println(ind);
}

void serial_init() {
  Serial1.begin(9600); // for communicating with Robotic Arm
  Serial2.begin(9600); // for communicating with Pi
  Serial.println("Host/Target serial communication initialized");
}
