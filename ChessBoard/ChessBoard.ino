#include <Adafruit_MCP23X17.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <SoftwareSerial.h>

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

state_t state;
String mov[2];
String legal[16];
uint8_t legal_moves;

#define BUTTON_PIN 0  // MCP23XXX pin button is attached to
#define PIN_LED 6
#define LED_ROWS 4
#define LED_COLUMNS 4
#define LED_COUNT (LED_ROWS * LED_COLUMNS)

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

const byte rxPin = 3; // Arduino RX = D3
const byte txPin = 2; // Arduino TX = D2

// Begin a software serial on pins D3 and D2 to talk to the Pi
SoftwareSerial mySerial (rxPin, txPin);

Adafruit_MCP23X17 mcp;
Adafruit_NeoMatrix led_matrix(LED_COLUMNS, LED_ROWS, PIN_LED, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + NEO_GRB + NEO_KHZ800);
uint8_t sensor_matrix[4][4] = {
  {15, 14, 13, 12},
  {11, 10,  9,  8},
  { 7,  6,  5,  4},
  { 3,  2,  1,  0},
};

String notation_matrix[4][4] = {
  {"a4", "b4", "c4", "d4"},
  {"a3", "b3", "c3", "d3"},
  {"a2", "b2", "c2", "d2"},
  {"a1", "b1", "c1", "d1"},
};

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

void show_count_up() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      led_matrix.drawPixel(i,j,ORANGE);
      led_matrix.show();
      delay(100);
    }
  }
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
  
  led_matrix.show();
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

int parse_command(String command, String tokens[], int max_tokens) {
    int index = 0;
    int token_count = 0;

    while (index != -1 && token_count < max_tokens) {
        // Find the next delimiter
        int found = command.indexOf(':', index);

        // Extract the substring
        tokens[token_count] = (found == -1) ? command.substring(index) : command.substring(index, found);

        // Increment the token count
        token_count++;

        // Move to the next part of the string
        index = (found == -1) ? -1 : found + 1;
    }

    return token_count; // Return the number of tokens found
}

void process_cmd(String cmd) {
  const int max_tokens = 10;
  String tokens[max_tokens];

  int num_tokens = parse_command(cmd, tokens, max_tokens);

  if (tokens[0].equals("scan")) {
    if (tokens[1].equals("start")) {
      state = SCAN_START;
    }
  } else if (tokens[0].equals("legal")) {
    legal_moves = num_tokens - 1;
    Serial.print("Legal down: ");
    for (int i=1; i<num_tokens; i++) {
      legal[i-1] = tokens[i];
      Serial.print(tokens[i]);
      Serial.print(", ");
    }
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}

String check_for_cmd() {
  String input = mySerial.readStringUntil('\n');
  if (input != NULL) {
    input.trim();
  }
  return input;
}

void wait_for_pi_to_start() {
  int chess_squares_lit = 0;
  while(true) {
    chess_squares_lit = loading_status(chess_squares_lit);
    delay(100);
    // If the Pi has sent data, read the data buffer until no more data
    if (mySerial.available()) {
      while (mySerial.available()) {
        Serial.print(mySerial.read(), HEX);
      } // Read from Pi, print to hardware Serial
      Serial.println();
      break;
    }
  }
}

void get_occupancy_matrix(bool om[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      if (!mcp.digitalRead(sensor_matrix[i][j])) {
        om[i][j] = true;
      } else {
        om[i][j] = false;
      }
    }
  }
}

void print_occupancy_matrix(bool om[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      Serial.print(om[i][j]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

void save_occupancy_matrix(bool omo[4][4], bool omn[4][4]) {
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      omo[i][j] = omn[i][j];
    }
  }
}

move_t detect_move(bool omo[4][4], bool omn[4][4], String &pos) {
  move_t move = MOVE_NONE;
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      if ((omn[i][j] - omo[i][j]) == 1) {
        //led_matrix.drawPixel(i,j,GREEN);
        notation_lookup(i, j, pos);
        move = MOVE_DOWN;
      } else if ((omn[i][j] - omo[i][j]) == -1) {
        //led_matrix.drawPixel(i,j,RED);
        move = MOVE_UP;
        notation_lookup(i, j, pos);
      } else {
        #if 0
        if ((i+j) % 2 == 0) {
          led_matrix.drawPixel(i,j,WHITE);
        } else {
          led_matrix.drawPixel(i,j,BLUE);
        }
        #endif
      }
    }
  }
  //led_matrix.show();

  return move;
}

bool om_old[4][4];
bool om_new[4][4];

void setup() {
  Serial.begin(9600);
  while (!Serial);
  mySerial.begin(9600); // Software serial for communicating with Pi
  Serial.println("Welcome to SmartChess!");

  // uncomment appropriate mcp.begin
  if (!mcp.begin_I2C(0x27)) {
    Serial.println("Error.");
    while (1);
  }

  // configure pin for input with pull up
  for (int i=BUTTON_PIN; i<BUTTON_PIN+16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }

  led_matrix.begin();
  led_matrix.setTextWrap(false);
  led_matrix.setBrightness(50);
  led_matrix.show();

  show_chessboard();
  //wait_for_pi_to_start();
  get_occupancy_matrix(om_old);
  state = SCAN_NONE;
  mov[0] = "";
  mov[1] = "";
}

void loop() {
  if (state != SCAN_NONE) {
    get_occupancy_matrix(om_new);
    String pos;
    move_t move = detect_move(om_old, om_new, pos);
    if (move == MOVE_UP) {
      mov[0] = pos;
      mov[1] = "";
      //mySerial.print("up:");
      mySerial.println(pos);
      Serial.print("UP: ");
      Serial.println(mov[0]);
    } else if (move == MOVE_DOWN) {
      mov[1] = pos;
      //mySerial.print("dn:");
      mySerial.println(pos);
      Serial.print("DOWN: ");
      Serial.println(mov[1]);
    }
    save_occupancy_matrix(om_old, om_new);
  }

  show_chessboard();

  if ((state == SCAN_NONE) || (state == SCAN_START)) {
    String cmd = check_for_cmd();
    if (cmd != NULL) {
      process_cmd(cmd);
    }
  }
}
