#ifndef UTILS_H
#define UTILS_H

typedef enum {
  SCAN_NONE,
  SCAN_START,
  SCAN_STOP,
} state_t;

typedef enum {
  MOVE_NONE,
  MOVE_UP,
  MOVE_DOWN,
} move_t;

extern String notation_matrix[4][4];

void notation_lookup(uint8_t i, uint8_t j, String &notation) {
  notation = notation_matrix[i][j];
}

void xy_lookup(String &notation, uint8_t &i, uint8_t &j) {
  for (uint8_t m=0; m<4; m++) {
    for (uint8_t n=0; n<4; n++) {
      if (notation.equals(notation_matrix[m][n])) {
        i = m;
        j = n;
        return;
      }
    }
  }
}

void show_count_up();
int loading_status(int chess_squares_already_lit);
void show_chessboard();
void display_init();
void get_occupancy_matrix(bool om[4][4]);
void print_occupancy_matrix(bool om[4][4]);
void save_occupancy_matrix(bool omo[4][4], bool omn[4][4]);
move_t detect_move(bool omo[4][4], bool omn[4][4], String &pos);
void sensor_init();
String check_for_cmd();
void serial_init();
int parse_command(String command, String tokens[], int max_tokens);
void process_cmd(String cmd);
void scan_buttons(bool &confirm, bool &hint);

#endif // UTILS_H
