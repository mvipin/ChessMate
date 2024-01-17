#ifndef UTILS_H
#define UTILS_H

typedef enum {
  MOVE_NONE,
  MOVE_INIT,
  MOVE_START,
  MOVE_UP,
  MOVE_DOWN,
  MOVE_STOP,
} state_t;

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
#define LEGAL_MOVES_MAX 21
#define CMD_LEN_MAX 128

extern char algebraic_notation[CHESS_ROWS][CHESS_COLS][3];

void algebraic_lookup(uint8_t i, uint8_t j, char *notation) {
  if (i < CHESS_ROWS && j < CHESS_COLS) {
    strncpy(notation, algebraic_notation[i][j], 3);
  }
}

void xy_lookup(const char *notation, uint8_t &i, uint8_t &j) {
  for (uint8_t m=0; m<CHESS_ROWS; m++) {
    for (uint8_t n=0; n<CHESS_COLS; n++) {
      if (strcmp(notation, algebraic_notation[m][n]) == 0) {
        i = m;
        j = n;
        return;
      }
    }
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
