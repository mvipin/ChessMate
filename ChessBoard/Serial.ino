#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Utils.h"

const byte rxPin = 3; // Arduino RX = D3
const byte txPin = 2; // Arduino TX = D2

// Begin a software serial on pins D3 and D2 to talk to the Pi
SoftwareSerial mySerial (rxPin, txPin);

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

void serial_init() {
  mySerial.begin(9600); // Software serial for communicating with Pi
}
