#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

servo_cfg_t servo_cfg[SERVO_NUM_MAX] = {
  {80, 530, 35, 100}, // SERVO_RIGHT_EYELID (0)
  {80, 530, 40, 120}, // SERVO_RIGHT_EYEBALL (1)
  {100, 500, 0, 180}, // SERVO_RIGHT_EYELEVEL (2)
  {80, 530, 80, 135}, // SERVO_LEFT_EYELID (3)
  {80, 530, 55, 140}, // SERVO_LEFT_EYEBALL (4)
  {100, 500, 0, 180}, // SERVO_LEFT_EYELEVEL (5)
};

void set_eyeball(dir_t dir, pos_clk_t clk)
{
  float factor;
  servo_id_t servo_id = (dir == RIGHT) ? SERVO_RIGHT_EYEBALL : SERVO_LEFT_EYEBALL;
  switch (clk) {
    case POS_3H: {
      factor = 1.00f;
    } break;
    case POS_2H: {
      factor = 0.83f;
    } break;
    case POS_1H: {
      factor = 0.66f;
    } break;
    case POS_12H: {
      factor = 0.50f;
    } break;
    case POS_11H: {
      factor = 0.34f;
    } break;
    case POS_10H: {
      factor = 0.17f;
    } break;
    case POS_9H: {
      factor = 0.00f;
    } break;
  }

  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
}

void set_eyelid(dir_t dir, pos_gap_t gap)
{
  float factor;
  servo_id_t servo_id = (dir == RIGHT) ? SERVO_RIGHT_EYELEVEL : SERVO_LEFT_EYELEVEL;
  switch (gap) {
    case FULLY_OPEN: {
      factor = (dir == RIGHT) ? 1.00f : 0.00f;
    } break;
    case PARTIALLY_OPEN: {
      factor = (dir == RIGHT) ? 0.75f : 0.25f;
    } break;
    case HALF_OPEN: {
      factor = 0.50f;
    } break;
    case PARTIALLY_CLOSED: {
      factor = (dir == RIGHT) ? 0.25f : 0.75f;
    } break;
    case FULLY_CLOSED: {
      factor = (dir == RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
}

void set_eyelevel(dir_t dir, pos_height_t level)
{
  float factor;
  servo_id_t servo_id = (dir == RIGHT) ? SERVO_RIGHT_EYELEVEL : SERVO_LEFT_EYELEVEL;
  switch (level) {
    case FULLY_DOWN: {
      factor = (dir == RIGHT) ? 1.00f : 0.00f;
    } break;
    case LOWER_TILTED: {
      factor = (dir == RIGHT) ? 0.83f : 0.17f;
    } break;
    case SLIGHTLY_TILTED_DOWN: {
      factor = (dir == RIGHT) ? 0.66f : 0.34f;
    } break;
    case NEUTRAL: {
      factor = 0.50f;
    } break;
    case SLIGHTLY_TILTED_UP: {
      factor = (dir == RIGHT) ? 0.34f : 0.66f;
    } break;
    case RAISED_TILTED: {
      factor = (dir == RIGHT) ? 0.17f : 0.83f;
    } break;
    case FULLY_UP: {
      factor = (dir == RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
}

void eyelids_test()
{
  for (int gap = FULLY_OPEN; gap <= FULLY_CLOSED; gap++) {
    set_eyelid(LEFT, gap);
    set_eyelid(RIGHT, gap);
    delay(500);
  }
  for (int gap = FULLY_CLOSED; gap >= FULLY_OPEN; gap--) {
    set_eyelid(LEFT, gap);
    set_eyelid(RIGHT, gap);
    delay(500);
  }
}

void eyeballs_test()
{
  set_eyelid(LEFT, FULLY_OPEN);
  set_eyelid(RIGHT, FULLY_OPEN);
  for (int clk = POS_3H; clk <= POS_9H; clk++) {
    set_eyeball(LEFT, clk);
    set_eyeball(RIGHT, clk);
    delay(500);
  }
  for (int clk = POS_9H; clk >= POS_3H; clk--) {
    set_eyeball(LEFT, clk);
    set_eyeball(RIGHT, clk);
    delay(500);
  }
}

void set_eyelevel_test()
{
  set_eyelid(LEFT, HALF_OPEN);
  set_eyelid(RIGHT, HALF_OPEN);
  for (int level = FULLY_DOWN; level <= FULLY_UP; level++) {
    set_eyelevel(LEFT, level);
    set_eyelevel(RIGHT, level);
    delay(1000);
  }
  for (int level = FULLY_UP; level >= FULLY_DOWN; level--) {
    set_eyelevel(LEFT, level);
    set_eyelevel(RIGHT, level);
    delay(1000);
  }
}

void head_init()
{
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  delay(10);
}