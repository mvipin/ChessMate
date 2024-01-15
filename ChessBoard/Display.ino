#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "Utils.h"

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

#define PIN_LED 6
#define PIN_TURN_LED 7
#define LED_ROWS 4
#define LED_COLUMNS 4
#define LED_COUNT (LED_ROWS * LED_COLUMNS)
#define NUMPIXELS 2

Adafruit_NeoMatrix led_matrix(LED_COLUMNS, LED_ROWS, PIN_LED, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel turn(NUMPIXELS, PIN_TURN_LED, NEO_GRB + NEO_KHZ800);

void show_count_up() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      led_matrix.drawPixel(i,j,ORANGE);
      led_matrix.show();
      delay(100);
    }
  }
}

int loading_status(int chess_squares_already_lit) {
  int target_row = 0;
  int target_column = 0;
  if (chess_squares_already_lit < 8){
    target_row = 0;
    target_column = chess_squares_already_lit;
  } else if (chess_squares_already_lit > 7){
    target_row = chess_squares_already_lit / 8;
    Serial.println(target_row);
    target_column = chess_squares_already_lit % 8;
    Serial.println(target_column);
    Serial.println();
  }
  uint16_t color = random(0, 0xFFFF);
  led_matrix.drawPixel(target_row,target_column,color);
  led_matrix.show();
  if ((target_row == 7) && (target_column == 7)) {
    return 0;
  }
  
  return chess_squares_already_lit + 1;
}

void show_chessboard() {
  for (int i = 0; i < 4; i=i+2) {
    for (int j = 0; j < 4; j++) {
      if (j % 2 == 0) {
        led_matrix.drawPixel(i,j,WHITE);
        led_matrix.drawPixel(i+1,j,BLUE);
      } else {
        led_matrix.drawPixel(i,j,BLUE);
        led_matrix.drawPixel(i+1,j,WHITE);
      }
    }
  }

  if (mov[0] != "") {
    uint8_t i, j;
    xy_lookup(mov[0], i, j);
    led_matrix.drawPixel(i,j,GREEN);
  }

  if (mov[1] != "") {
    uint8_t i, j;
    xy_lookup(mov[1], i, j);
    led_matrix.drawPixel(i,j,RED);
  }

  if (legal_moves) {
    for (int k=0; k<legal_moves; k++) {
      uint8_t i, j;
      xy_lookup(legal[k], i, j);
      led_matrix.drawPixel(i,j,MAGENTA);
    }
  }

  turn.setPixelColor(0, turn.Color(0, 150, 0));
  turn.setPixelColor(1, turn.Color(150, 0, 0));

  led_matrix.show();
  turn.show();
}

void display_init() {
  led_matrix.begin();
  led_matrix.setTextWrap(false);
  led_matrix.setBrightness(50);
  led_matrix.show();
  turn.begin();
  turn.clear();
  turn.setBrightness(10);
}
