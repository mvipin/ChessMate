#ifndef UTILS_H
#define UTILS_H

typedef enum {
  MOVE_NONE,
  MOVE_INIT,
  MOVE_RESET,
  MOVE_START,
  MOVE_STOP,
  MOVE_COMP,
} state_t;

enum {
  MOVE_TYPE_COMP,
  MOVE_TYPE_HINT,
  MOVE_TYPE_MAX,
};

#define COMPUTER 0
#define HUMAN 1

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define ORANGE   0xFC00
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF 

#define CHESS_ROWS 8
#define CHESS_COLS 8
#define LEGAL_MOVES_MAX 32
#define CMD_LEN_MAX 128

void get_algebraic_notation(int row, int col, char *notation) {
    if (row >= 0 && row < CHESS_ROWS && col >= 0 && col < CHESS_COLS) {
        notation[0] = 'a' + col; // Columns map to letters
        notation[1] = '1' + (CHESS_ROWS - 1 - row); // Rows map to numbers, inverted
        notation[2] = '\0'; // Null-terminate the string
    }
}

void xy_lookup(const char *notation, uint8_t &row, uint8_t &col) {
    if (notation[0] >= 'a' && notation[0] <= 'h' && notation[1] >= '1' && notation[1] <= '8') {
        col = notation[0] - 'a'; // Convert letter to column
        row = CHESS_ROWS - (notation[1] - '1') - 1; // Convert number to row and invert
    }
}

void print_matrix(int8_t matrix[CHESS_ROWS][CHESS_COLS]) {
  for (int i=0; i<CHESS_ROWS; i++) {
    for (int j=0; j<CHESS_COLS; j++) {
      Serial.print(matrix[i][j]);
      Serial.print("\t");
    }
    Serial.println();
  }
}
#endif // UTILS_H