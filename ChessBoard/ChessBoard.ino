#include <SoftwareSerial.h>
#include "Utils.h"

state_t state;
String mov[2];
String legal[16];
uint8_t legal_moves;
extern SoftwareSerial mySerial;
bool btn_hint;
bool btn_confirm;

String notation_matrix[4][4] = {
  {"a4", "b4", "c4", "d4"},
  {"a3", "b3", "c3", "d3"},
  {"a2", "b2", "c2", "d2"},
  {"a1", "b1", "c1", "d1"},
};

bool om_old[4][4];
bool om_new[4][4];

void scan_sensors() {
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
}

void scan_serial() {
  if ((state == SCAN_NONE) || (state == SCAN_START)) {
    String cmd = check_for_cmd();
    if (cmd != NULL) {
      process_cmd(cmd);
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  serial_init();
  sensor_init();
  button_init();
  display_init();

  show_chessboard();
  
  //wait_for_pi_to_start();
  get_occupancy_matrix(om_old);
  state = SCAN_NONE;
  mov[0] = "";
  mov[1] = "";

  Serial.println("Welcome to SmartChess!");
}

void loop() {
  scan_sensors();
  show_chessboard();
  //scan_serial();
  scan_buttons(btn_confirm, btn_hint);
  if (btn_confirm == true) {
    Serial.println("confirm");
  } else if (btn_hint == true) {
    Serial.println("hint");
  }
}
