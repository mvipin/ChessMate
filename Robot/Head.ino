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
  servo_id_t servo_id = (dir == EYE_RIGHT) ? SERVO_RIGHT_EYEBALL : SERVO_LEFT_EYEBALL;
  switch (clk) {
    case FULLY_RIGHT: {
      factor = 1.00f;
    } break;
    case PARTIALLY_RIGHT: {
      factor = 0.83f;
    } break;
    case SLIGHTLY_RIGHT: {
      factor = 0.66f;
    } break;
    case CENTER: {
      factor = 0.50f;
    } break;
    case SLIGHTLY_LEFT: {
      factor = 0.34f;
    } break;
    case PARTIALLY_LEFT: {
      factor = 0.17f;
    } break;
    case FULLY_LEFT: {
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
  servo_id_t servo_id = (dir == EYE_RIGHT) ? SERVO_RIGHT_EYELID : SERVO_LEFT_EYELID;
  switch (gap) {
    case FULLY_OPEN: {
      factor = (dir == EYE_RIGHT) ? 1.00f : 0.00f;
    } break;
    case PARTIALLY_OPEN: {
      factor = (dir == EYE_RIGHT) ? 0.75f : 0.25f;
    } break;
    case HALF_OPEN: {
      factor = 0.50f;
    } break;
    case PARTIALLY_CLOSED: {
      factor = (dir == EYE_RIGHT) ? 0.25f : 0.75f;
    } break;
    case FULLY_CLOSED: {
      factor = (dir == EYE_RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
}

void set_eyelevel(dir_t dir, pos_level_t level)
{
  float factor;
  servo_id_t servo_id = (dir == EYE_RIGHT) ? SERVO_RIGHT_EYELEVEL : SERVO_LEFT_EYELEVEL;
  switch (level) {
    case FULLY_DOWN: {
      factor = (dir == EYE_RIGHT) ? 1.00f : 0.00f;
    } break;
    case LOWER_TILTED: {
      factor = (dir == EYE_RIGHT) ? 0.83f : 0.17f;
    } break;
    case SLIGHTLY_TILTED_DOWN: {
      factor = (dir == EYE_RIGHT) ? 0.66f : 0.34f;
    } break;
    case NEUTRAL: {
      factor = 0.50f;
    } break;
    case SLIGHTLY_TILTED_UP: {
      factor = (dir == EYE_RIGHT) ? 0.34f : 0.66f;
    } break;
    case RAISED_TILTED: {
      factor = (dir == EYE_RIGHT) ? 0.17f : 0.83f;
    } break;
    case FULLY_UP: {
      factor = (dir == EYE_RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
}

void eyelids_test()
{
  for (int gap = FULLY_OPEN; gap <= FULLY_CLOSED; gap++) {
    set_eyelid(EYE_LEFT, gap);
    set_eyelid(EYE_RIGHT, gap);
    delay(500);
  }
  for (int gap = FULLY_CLOSED; gap >= FULLY_OPEN; gap--) {
    set_eyelid(EYE_LEFT, gap);
    set_eyelid(EYE_RIGHT, gap);
    delay(500);
  }
}

void eyeballs_test()
{
  set_eyelid(EYE_LEFT, FULLY_OPEN);
  set_eyelid(EYE_RIGHT, FULLY_OPEN);
  for (int clk = FULLY_RIGHT; clk <= FULLY_LEFT; clk++) {
    set_eyeball(EYE_LEFT, clk);
    set_eyeball(EYE_RIGHT, clk);
    delay(500);
  }
  for (int clk = FULLY_LEFT; clk >= FULLY_RIGHT; clk--) {
    set_eyeball(EYE_LEFT, clk);
    set_eyeball(EYE_RIGHT, clk);
    delay(500);
  }
}

void eyelevel_test()
{
  set_eyelid(EYE_LEFT, HALF_OPEN);
  set_eyelid(EYE_RIGHT, HALF_OPEN);
  for (int level = FULLY_DOWN; level <= FULLY_UP; level++) {
    set_eyelevel(EYE_LEFT, level);
    set_eyelevel(EYE_RIGHT, level);
    delay(1000);
  }
  for (int level = FULLY_UP; level >= FULLY_DOWN; level--) {
    set_eyelevel(EYE_LEFT, level);
    set_eyelevel(EYE_RIGHT, level);
    delay(1000);
  }
}

void expression_1()
{
  set_eyelid(EYE_LEFT, FULLY_OPEN);
  set_eyelid(EYE_RIGHT, FULLY_OPEN);
  set_eyeball(EYE_LEFT, CENTER);
  set_eyeball(EYE_RIGHT, CENTER);
  set_eyelevel(EYE_LEFT, SLIGHTLY_TILTED_DOWN);
  set_eyelevel(EYE_RIGHT, SLIGHTLY_TILTED_UP);
  delay(700);
  set_eyelevel(EYE_LEFT, NEUTRAL);
  set_eyelevel(EYE_RIGHT, NEUTRAL);
  delay(700);
  set_eyelevel(EYE_LEFT, SLIGHTLY_TILTED_UP);
  set_eyelevel(EYE_RIGHT, SLIGHTLY_TILTED_DOWN);
  delay(700);
  set_eyelevel(EYE_RIGHT, NEUTRAL);
  set_eyelevel(EYE_LEFT, NEUTRAL);
  delay(700);
  set_eyeball(EYE_RIGHT, FULLY_RIGHT);
  set_eyeball(EYE_LEFT, FULLY_RIGHT);
  delay(700);
  set_eyeball(EYE_RIGHT, CENTER);
  set_eyeball(EYE_LEFT, CENTER);
  delay(700);
  set_eyeball(EYE_RIGHT, FULLY_LEFT);
  set_eyeball(EYE_LEFT, FULLY_LEFT);
  delay(700);
  set_eyeball(EYE_RIGHT, CENTER);
  set_eyeball(EYE_LEFT, CENTER);
  delay(700);
}

void head_init()
{
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  delay(10);
}