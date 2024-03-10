#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>
#include <assert.h>
#include <math.h>
#include <EEPROM.h>

#define MULTISTEPPER_ENABLED //change to #undef if we want the motors to accelerate/decelerate

#define STEPPER_X_STP_PIN 2
#define STEPPER_Y_STP_PIN 3 
#define STEPPER_Z_STP_PIN 4
#define STEPPER_X_DIR_PIN 5 
#define STEPPER_Y_DIR_PIN 6
#define STEPPER_Z_DIR_PIN 7
#define STEPPER_ENABLE_PIN 8
#define LIMIT_SWITCH_X_PIN 9
#define LIMIT_SWITCH_Y_PIN 10
#define LIMIT_SWITCH_Z_PIN 11

#define GRIPPER_SERVO_PIN 13
#define GRIPPER_OPEN_ANGLE 30
#define GRIPPER_CLOSE_ANGLE 100

#define MOTOR_INTERFACE_TYPE 1

#define ARM_1_SIZE 202.0f // mm
#define ARM_2_SIZE 190.0f // mm
#define LOWER_LEFT_CORNER_X -116.65 // origin at base
#define LOWER_LEFT_CORNER_Y 137.0
#define UPPER_LEFT_CORNER_X -116.65
#define UPPER_LEFT_CORNER_Y 370.33 // 137 + 233.33
#define UPPER_RIGHT_CORNER_X 116.65
#define UPPER_RIGHT_CORNER_Y 370.33
#define LOWER_RIGHT_CORNER_X 116.65
#define LOWER_RIGHT_CORNER_Y 137.0
#define CHESS_ROWS 8
#define CHESS_COLS 8

#define EEPROM_START_ADDRESS 0
#define ANGLE_DATA_SIZE 8 // bytes per square

#define MAX_SPEED_X 2000
#define REDUCED_SPEED_X (MAX_SPEED_X >> 2)
#define HOMING_SPEED_X (-MAX_SPEED_X/2)
#define ACCELERATION_X 1000

#define MAX_SPEED_Y 4000
#define REDUCED_SPEED_Y (MAX_SPEED_Y >> 2)
#define HOMING_SPEED_Y (-MAX_SPEED_Y/2)
#define ACCELERATION_Y 1000

#define MAX_SPEED_Z 4000
#define HOMING_SPEED_Z -1000
#define ACCELERATION_Z 1000
#define Z_MIN 250
#define Z_MAX 2450

#define STEPS_REF_X 440
#define STEPS_REF_Y 7100
#define STEPS_PER_DEGREE_X 32
#define STEPS_PER_DEGREE_Y 51.7769
#define STEPS_CNT(_deg, _dir) \
  ((_deg * STEPS_PER_DEGREE_##_dir) + STEPS_REF_##_dir)
#define STEP_ZERO_ANGLE(_dir) \
  (-STEPS_REF_##_dir / STEPS_PER_DEGREE_##_dir)

#define CAL_LARGE_STEP 5.0
#define CAL_SMALL_STEP 0.5

AccelStepper stepperX = AccelStepper(MOTOR_INTERFACE_TYPE, STEPPER_X_STP_PIN, STEPPER_X_DIR_PIN);  
AccelStepper stepperY = AccelStepper(MOTOR_INTERFACE_TYPE, STEPPER_Y_STP_PIN, STEPPER_Y_DIR_PIN);  
AccelStepper stepperZ = AccelStepper(MOTOR_INTERFACE_TYPE, STEPPER_Z_STP_PIN, STEPPER_Z_DIR_PIN);  

MultiStepper multistepper;
long multistepper_positions[2];
Servo gripper;
double angle1 = 0.0;
double angle2 = 0.0;
int current_square_index = 0; // From 0 to 63 for the 64 squares

struct grid {
  double x;
  double y;
} corner[] = {
  {LOWER_LEFT_CORNER_X,LOWER_LEFT_CORNER_Y},
  {UPPER_LEFT_CORNER_X,UPPER_LEFT_CORNER_Y},
  {UPPER_RIGHT_CORNER_X,UPPER_RIGHT_CORNER_Y},
  {LOWER_RIGHT_CORNER_X,LOWER_RIGHT_CORNER_Y},
};

void gripper_close()
{
  gripper.write(GRIPPER_CLOSE_ANGLE); 
}

void gripper_open()
{
  gripper.write(GRIPPER_OPEN_ANGLE);
}

void move_xy_to(double x_deg, double y_deg)
{
  long x_steps = STEPS_CNT(x_deg, X);
  long y_steps = STEPS_CNT(y_deg, Y);
  //Serial.print(x_steps);
  //Serial.print(", ");
  //Serial.println(y_steps);

#ifdef MULTISTEPPER_ENABLED
  multistepper_positions[0] = x_steps;
  multistepper_positions[1] = y_steps;
  multistepper.moveTo(multistepper_positions);
  multistepper.runSpeedToPosition(); // Blocks until all are in position
#else
  stepperX.moveTo(x_steps);
  stepperY.moveTo(y_steps);
  while ((stepperX.currentPosition() != x_steps) || (stepperY.currentPosition() != y_steps)) {
    stepperX.run();
    stepperY.run();
  }
#endif
}

void move_x_to(double degree)
{
  long steps = STEPS_CNT(degree, X);
  stepperX.moveTo(steps);
  while (stepperX.currentPosition() != steps) {
    stepperX.run();
  }
}

void move_y_to(double degree)
{
  long steps = STEPS_CNT(degree, Y);
  stepperY.moveTo(steps);
  while (stepperY.currentPosition() != steps) {
    stepperY.run();
  }
}

void move_z_to(long steps)
{
  stepperZ.moveTo(steps);
  while (stepperZ.currentPosition() != steps) {
    stepperZ.run();
  }
}

void home_x()
{
  stepperX.setSpeed(HOMING_SPEED_X);
  while (!digitalRead(LIMIT_SWITCH_X_PIN)) {
    stepperX.runSpeed();
  }
  stepperX.setCurrentPosition(0); // When limit switch pressed set position to 0 steps
}

void home_y()
{
  stepperY.setSpeed(HOMING_SPEED_Y);
  while (!digitalRead(LIMIT_SWITCH_Y_PIN)) {
    stepperY.runSpeed();
  }
  stepperY.setCurrentPosition(0); // When limit switch pressed set position to 0 steps
}

void home_all()
{
  home_z();
  move_z_to(Z_MAX);
  home_x();
  move_x_to(90);
  home_y();
  move_y_to(0);
}

void home_z()
{
  stepperZ.setSpeed(HOMING_SPEED_Z);
  while (!digitalRead(LIMIT_SWITCH_Z_PIN)) {
    stepperZ.runSpeed();
  }
  stepperZ.setCurrentPosition(0); // When limit switch pressed set position to 0 steps
}

void inverse_kinematics(double x, double y, double *theta1, double *theta2)
{
  double c = sqrt(x * x + y * y);
  //Serial.print("c: "); Serial.println(c);
  double b2 = atan2(x, y) * (180/3.1416);
  //Serial.print("B2: "); Serial.println(b2);
  double B = acos(((ARM_1_SIZE * ARM_1_SIZE) + (x * x) + (y * y) - (ARM_2_SIZE * ARM_2_SIZE)) / (2 * c * ARM_1_SIZE)) * (180/3.1416);
  //Serial.print("B: "); Serial.println(B);
  double b1 = B + b2;
  //Serial.print("B1: "); Serial.println(b1);
  *theta1 = 90 - b1;

  double C = acos(((ARM_1_SIZE * ARM_1_SIZE) + (ARM_2_SIZE * ARM_2_SIZE) - (c * c))/(2 * ARM_1_SIZE * ARM_2_SIZE)) * (180/3.1416);
  *theta2 = 180 - C;
  Serial.print("theta1: "); Serial.println(*theta1);
  Serial.print("theta2: "); Serial.println(*theta2);
  Serial.println();
}

void four_corner_test()
{
  double theta1, theta2;

  for (int i=0; i<4; i++) {
    inverse_kinematics(corner[i].x, corner[i].y, &theta1, &theta2);
    move_xy_to(theta1, theta2 - ((90-theta1)/1.879));
    gripper_close();
    move_z_to(Z_MIN);
    gripper_open();
    delay(2000);
    gripper_close();
    move_z_to(Z_MAX);
  }
}

void up_down_test()
{
  stepperZ.setSpeed(MAX_SPEED_Z);
  long t0 = millis();
  move_z_to(Z_MAX);
  long t1 = millis();
  Serial.print("Up: ");
  Serial.println(t1-t0);
  delay(500);
  t0 = millis();
  move_z_to(0);
  t1 = millis();
  Serial.print("Down: ");
  Serial.println(t1-t0);
  delay(500);
}

void extraction_test()
{
  static uint8_t iteration = 0;
  uint8_t first = iteration + 1;
  uint8_t second = ((iteration + 1) % 2) + 1;
  move_xy_to(corner[first].x, (-(90-corner[first].x)/1.879) + (180-corner[first].y));
  gripper_open();
  move_z_to(Z_MIN);
  gripper_close();
  move_z_to(Z_MAX);
  move_xy_to(corner[second].x, (-(90-corner[second].x)/1.879) + (180-corner[second].y));
  move_z_to(Z_MIN);
  gripper_open();
  move_z_to(Z_MAX);
  gripper_close();
  iteration = (iteration + 1) % 2; 
}

void curl_up()
{
  move_y_to(0);
  move_x_to(STEP_ZERO_ANGLE(X));
  move_y_to(STEP_ZERO_ANGLE(Y) + ((STEP_ZERO_ANGLE(X) - 90)/1.879));
}

void dump_eeprom()
{
  float angles[2];
  int address = EEPROM_START_ADDRESS;
  
  for (int i = 0; i < 64; i++) {
    // Read each angle for the square
    for (int j = 0; j < 2; j++) {
      EEPROM.get(address, angles[j]);
      address += sizeof(float);
    }
    // Print the angles
    Serial.print("Square ");
    Serial.print(i);
    Serial.print(": Angle 1 = ");
    Serial.print(angles[0], 6); // Print with precision of 6 digits
    Serial.print(", Angle 2 = ");
    Serial.println(angles[1], 6); // Print with precision of 6 digits
  }
}

void store_angles_for_square()
{
  int eeprom_address = EEPROM_START_ADDRESS + current_square_index * ANGLE_DATA_SIZE;
  EEPROM.put(eeprom_address, angle1);
  EEPROM.put(eeprom_address + sizeof(float), angle2);
  Serial.print("Stored angles for square ");
  Serial.println(current_square_index);
}

void adjust_angle(double* angle, double step)
{
  *angle += step;
  // Add bounds checking for angles if needed
}

void xy_lookup(uint8_t index, double *x, double *y)
{
  // Calculate row and column from index
  int row = current_square_index / CHESS_COLS;
  int col = current_square_index % CHESS_COLS;

  // Calculate fractions along each axis
  double x_fraction = (double)col / (CHESS_COLS - 1);
  double y_fraction = (double)row / (CHESS_ROWS - 1);

  // Interpolate x and y positions
  double x_left = LOWER_LEFT_CORNER_X * (1 - y_fraction) + UPPER_LEFT_CORNER_X * y_fraction;
  double x_right = LOWER_RIGHT_CORNER_X * (1 - y_fraction) + UPPER_RIGHT_CORNER_X * y_fraction;
  *x = x_left * (1 - x_fraction) + x_right * x_fraction;

  double y_lower = LOWER_LEFT_CORNER_Y * (1 - x_fraction) + LOWER_RIGHT_CORNER_Y * x_fraction;
  double y_upper = UPPER_LEFT_CORNER_Y * (1 - x_fraction) + UPPER_RIGHT_CORNER_Y * x_fraction;
  *y = y_lower * (1 - y_fraction) + y_upper * y_fraction;
}

void prompt_next_square()
{
  // Handling it this way because the board is upside down from robot's perspective
  const char* columns = "hgfedcba"; // Inverted order
  const char* rows = "12345678"; // Standard order, but will be accessed in reverse

  // Calculate file (column) and rank (row) from the currentSquareIndex
  int file_index = current_square_index % 8; // Column
  int rank_index = 7 - (current_square_index / 8); // Row, inverted

  // Construct the square notation
  char square_notation[3] = {columns[file_index], rows[rank_index], '\0'}; // Null-terminated string

  // Prompt the user
  Serial.print("Please jog to the square ");
  Serial.print(square_notation);
  Serial.println(" for adjustment. Press 's' to save when done.");

  // Nove the gripper to the desired position
  double x, y, theta1, theta2;
  xy_lookup(current_square_index, &x, &y);
  inverse_kinematics(x, y, &theta1, &theta2);
  move_xy_to(theta1, theta2 - ((90-theta1)/1.879));
  angle1 = theta1;
  angle2 = theta2;
}

bool handle_input(char input)
{
  // Increase angle1
  switch(input) {
    case 'L': {
      adjust_angle(&angle1, CAL_LARGE_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'R': {
      adjust_angle(&angle1, -CAL_LARGE_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'l': {
      adjust_angle(&angle1, CAL_SMALL_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'r': {
      adjust_angle(&angle1, -CAL_SMALL_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'D': {
      adjust_angle(&angle2, CAL_LARGE_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'U': {
      adjust_angle(&angle2, -CAL_LARGE_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'd': {
      adjust_angle(&angle2, CAL_SMALL_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 'u': {
      adjust_angle(&angle2, -CAL_SMALL_STEP);
      move_xy_to(angle1, angle2 - ((90-angle1)/1.879));
    } break;
    case 's': {
      store_angles_for_square();
      current_square_index++;
      if (current_square_index >= 64) {
        Serial.println("Calibration complete!");
        current_square_index = 0; // Reset or end calibration
      } else {
        prompt_next_square();
      }
    } break;
    case 'q': {
      Serial.println("Exiting calibration");
      dump_eeprom();
      return false;
    } break;
  }
  
  // Output current angles
  Serial.print("Angle1: ");
  Serial.print(angle1);
  Serial.print(", Angle2: ");
  Serial.println(angle2);

  return true;
}

void calibrate_board()
{
  stepperX.setMaxSpeed(REDUCED_SPEED_X);
  stepperY.setMaxSpeed(REDUCED_SPEED_Y);
  gripper_open();
  delay(2000);
  gripper_close();
  prompt_next_square();
  move_z_to(Z_MIN);
  while (true) {
    if (Serial.available() > 0) {
      char input = Serial.read();
      if (!(handle_input(input))) {
        break;
      }
    }
  }
  stepperX.setMaxSpeed(MAX_SPEED_X);
  stepperY.setMaxSpeed(MAX_SPEED_Y);
}

void setup()
{
  Serial.begin(9600);
  static_assert(GRIPPER_OPEN_ANGLE < GRIPPER_CLOSE_ANGLE, "Invalid gripper angle range");

  gripper.attach(GRIPPER_SERVO_PIN);

  stepperX.setMaxSpeed(MAX_SPEED_X);
  stepperX.setAcceleration(ACCELERATION_X);
  stepperX.setPinsInverted(false, false, true);
  stepperX.setEnablePin(STEPPER_ENABLE_PIN);

  stepperY.setMaxSpeed(MAX_SPEED_Y);
  stepperY.setAcceleration(ACCELERATION_Y);
  stepperY.setPinsInverted(false, false, true);
  stepperY.setEnablePin(STEPPER_ENABLE_PIN);

  stepperZ.setMaxSpeed(MAX_SPEED_Z);
  stepperZ.setAcceleration(ACCELERATION_Z);
  stepperZ.setPinsInverted(false, false, true);
  stepperZ.setEnablePin(STEPPER_ENABLE_PIN);
  stepperZ.setSpeed(MAX_SPEED_Z);

  multistepper.addStepper(stepperX);
  multistepper.addStepper(stepperY);

  pinMode(LIMIT_SWITCH_X_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_Y_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_Z_PIN, INPUT_PULLUP);

  home_all();

  calibrate_board();
}

void loop()
{
}