#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "Utils.h"

#define DISPLAY_LED_PIN 6
#define CONTROL_LED_PIN 7
#define DISPLAY_LED_CNT (CHESS_ROWS * CHESS_COLS)
#define CONTROL_LED_CNT 2

Adafruit_NeoMatrix display_pixels(CHESS_COLS, CHESS_ROWS, DISPLAY_LED_PIN, NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG + NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel control_pixel(CONTROL_LED_CNT, CONTROL_LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t display_map[CHESS_ROWS][CHESS_COLS] = {
  {16, 16, 16, 16, 16, 16, 16, 16},
  {16, 16, 16, 16, 16, 16, 16, 16},
  {16, 16, 16, 16, 16, 16, 16, 16},
  {16, 16, 16, 16, 16, 16, 16, 16},
  {15, 14, 13, 12, 16, 16, 16, 16},
  { 8,  9, 10, 11, 16, 16, 16, 16},
  { 7,  6,  5,  4, 16, 16, 16, 16},
  { 0,  1,  2,  3, 16, 16, 16, 16},
};

uint16_t remap_fn(uint16_t x, uint16_t y) {
  return display_map[x][y];
}

void show_count_up() {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      display_pixels.drawPixel(i,j,ORANGE);
      display_pixels.show();
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
    target_column = chess_squares_already_lit % 8;
  }
  uint16_t color = random(0, 0xFFFF);
  display_pixels.drawPixel(target_row,target_column,color);
  display_pixels.show();
  if ((target_row == 7) && (target_column == 7)) {
    return 0;
  }
  
  return chess_squares_already_lit + 1;
}

void reset_display() {
  for (uint8_t i = 0; i < CHESS_ROWS; i=i+2) {
    for (uint8_t j = 0; j < CHESS_COLS; j++) {
      if (j % 2 == 0) {
        display_pixels.drawPixel(i,j,WHITE);
        display_pixels.drawPixel(i+1,j,BLUE);
      } else {
        display_pixels.drawPixel(i,j,BLUE);
        display_pixels.drawPixel(i+1,j,WHITE);
      }
    }
  }
}

void lightup_display() {
  display_pixels.show();
}

void update_display(char square[], uint16_t color) {
  uint8_t i, j;
  xy_lookup(square, i, j);
  display_pixels.drawPixel(i,j, color);
}

void set_control_pixel(uint8_t idx, uint16_t color) {
  control_pixel.setPixelColor(idx, control_pixel.Color(0, color, 0));
  control_pixel.show();
}

void display_init() {
  display_pixels.begin();
  display_pixels.setTextWrap(false);
  display_pixels.setBrightness(50);
  control_pixel.begin();
  control_pixel.clear();
  control_pixel.setBrightness(10);
  display_pixels.setRemapFunction(remap_fn);
}
