#include <SoftwareSerial.h>
#include "Utils.h"

state_t state;
String mov[2];
String legal[16];
uint8_t legal_moves;
extern SoftwareSerial mySerial;

String notation_matrix[4][4] = {
  {"a4", "b4", "c4", "d4"},
  {"a3", "b3", "c3", "d3"},
  {"a2", "b2", "c2", "d2"},
  {"a1", "b1", "c1", "d1"},
};

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

bool om_old[4][4];
bool om_new[4][4];

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Welcome to SmartChess!");

  serial_init();
  sensor_init();
  display_init();

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
