#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

servo_cfg_t servo_cfg[SERVO_NUM_MAX] = {
  {80, 530, 35, 100, 67}, // SERVO_RIGHT_EYELID (0)
  {80, 530, 40, 120, 80}, // SERVO_RIGHT_EYEBALL (1)
  {100, 500, 0, 180, 90}, // SERVO_RIGHT_EYELEVEL (2)
  {80, 530, 80, 135, 107}, // SERVO_LEFT_EYELID (3)
  {80, 530, 55, 140, 97}, // SERVO_LEFT_EYEBALL (4)
  {100, 500, 0, 180, 90}, // SERVO_LEFT_EYELEVEL (5)
};

void set_eyeball(eye_t eye, pos_clk_t clk)
{
  float factor;
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

  servo_id_t servo_id = (eye == EYE_RIGHT) ? SERVO_RIGHT_EYEBALL : SERVO_LEFT_EYEBALL;
  uint16_t degrees = (uint16_t)(servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor));
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);

  servo_cfg[servo_id].degree_cur = degrees;
}

void set_eyelid(eye_t eye, pos_gap_t gap)
{
  float factor;
  switch (gap) {
    case FULLY_OPEN: {
      factor = (eye == EYE_RIGHT) ? 1.00f : 0.00f;
    } break;
    case PARTIALLY_OPEN: {
      factor = (eye == EYE_RIGHT) ? 0.75f : 0.25f;
    } break;
    case HALF_OPEN: {
      factor = 0.50f;
    } break;
    case PARTIALLY_CLOSED: {
      factor = (eye == EYE_RIGHT) ? 0.25f : 0.75f;
    } break;
    case FULLY_CLOSED: {
      factor = (eye == EYE_RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  servo_id_t servo_id = (eye == EYE_RIGHT) ? SERVO_RIGHT_EYELID : SERVO_LEFT_EYELID;
  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
  servo_cfg[servo_id].degree_cur = degrees;
}

void set_eyelevel(eye_t eye, pos_level_t level)
{
  float factor;
  switch (level) {
    case FULLY_DOWN: {
      factor = (eye == EYE_RIGHT) ? 1.00f : 0.00f;
    } break;
    case LOWER_TILTED: {
      factor = (eye == EYE_RIGHT) ? 0.83f : 0.17f;
    } break;
    case SLIGHTLY_TILTED_DOWN: {
      factor = (eye == EYE_RIGHT) ? 0.66f : 0.34f;
    } break;
    case NEUTRAL: {
      factor = 0.50f;
    } break;
    case SLIGHTLY_TILTED_UP: {
      factor = (eye == EYE_RIGHT) ? 0.34f : 0.66f;
    } break;
    case RAISED_TILTED: {
      factor = (eye == EYE_RIGHT) ? 0.17f : 0.83f;
    } break;
    case FULLY_UP: {
      factor = (eye == EYE_RIGHT) ? 0.00f : 1.00f;
    } break;
  }

  servo_id_t servo_id = (eye == EYE_RIGHT) ? SERVO_RIGHT_EYELEVEL : SERVO_LEFT_EYELEVEL;
  float degrees = servo_cfg[servo_id].degree_low + ((servo_cfg[servo_id].degree_high - servo_cfg[servo_id].degree_low) * factor);
  uint16_t pulse_len = map(degrees, 0, 180, servo_cfg[servo_id].pulse_min, servo_cfg[servo_id].pulse_max);
  pwm.setPWM(servo_id, 0, pulse_len);
  servo_cfg[servo_id].degree_cur = degrees;
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
  static uint32_t previousMillis = 0;
  static int step = 0;
  uint32_t currentMillis = millis();

  switch (step) {
    case 0: {
      if (currentMillis - previousMillis >= 700) {
        set_eyelid(EYE_LEFT, FULLY_OPEN);
        set_eyelid(EYE_RIGHT, FULLY_OPEN);
        set_eyeball(EYE_LEFT, CENTER);
        set_eyeball(EYE_RIGHT, CENTER);
        set_eyelevel(EYE_LEFT, SLIGHTLY_TILTED_DOWN);
        set_eyelevel(EYE_RIGHT, SLIGHTLY_TILTED_UP);
        previousMillis = currentMillis;
        step = 1;
      }
    } break;
    case 1: {
      if (currentMillis - previousMillis >= 700) {
        set_eyelevel(EYE_LEFT, NEUTRAL);
        set_eyelevel(EYE_RIGHT, NEUTRAL);
        previousMillis = currentMillis;
        step = 2;
      }
    }  break;
    case 2: {
      if (currentMillis - previousMillis >= 700) {
        set_eyelevel(EYE_LEFT, SLIGHTLY_TILTED_UP);
        set_eyelevel(EYE_RIGHT, SLIGHTLY_TILTED_DOWN);
        previousMillis = currentMillis;
        step = 3;
      }
    } break;
    case 3: {
      if (currentMillis - previousMillis >= 700) {
        set_eyelevel(EYE_RIGHT, NEUTRAL);
        set_eyelevel(EYE_LEFT, NEUTRAL);
        previousMillis = currentMillis;
        step = 4;
      }
    } break;
    case 4: {
      if (currentMillis - previousMillis >= 700) {
        set_eyeball(EYE_RIGHT, FULLY_RIGHT);
        set_eyeball(EYE_LEFT, FULLY_RIGHT);
        previousMillis = currentMillis;
        step = 5;
      }
    } break;
    case 5: {
      if (currentMillis - previousMillis >= 700) {
        set_eyeball(EYE_RIGHT, CENTER);
        set_eyeball(EYE_LEFT, CENTER);
        previousMillis = currentMillis;
        step = 6;
      }
    } break;
    case 6: {
      if (currentMillis - previousMillis >= 700) {
        set_eyeball(EYE_RIGHT, FULLY_LEFT);
        set_eyeball(EYE_LEFT, FULLY_LEFT);
        previousMillis = currentMillis;
        step = 7;
      }
    } break;
    case 7: {
      if (currentMillis - previousMillis >= 700) {
        set_eyeball(EYE_RIGHT, CENTER);
        set_eyeball(EYE_LEFT, CENTER);
        previousMillis = currentMillis;
        step = 0; // Reset state to 0 or remove this to stop repeating
      }
    } break;
  }
}

void expression_2()
{
#define DELAY 300

  // Write your code here
  set_eyelevel(EYE_RIGHT, FULLY_DOWN);
  set_eyelevel(EYE_LEFT, FULLY_DOWN);
  set_eyelid(EYE_RIGHT, PARTIALLY_CLOSED);
  set_eyelid(EYE_LEFT, PARTIALLY_CLOSED);
  set_eyeball(EYE_RIGHT, CENTER);
  set_eyeball(EYE_LEFT, CENTER);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, SLIGHTLY_LEFT);
  set_eyeball(EYE_LEFT, SLIGHTLY_LEFT);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, PARTIALLY_LEFT);
  set_eyeball(EYE_LEFT, PARTIALLY_LEFT);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, SLIGHTLY_LEFT);
  set_eyeball(EYE_LEFT, SLIGHTLY_LEFT);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, CENTER);
  set_eyeball(EYE_LEFT, CENTER); 
  delay(DELAY);
  set_eyeball(EYE_RIGHT, SLIGHTLY_RIGHT);
  set_eyeball(EYE_LEFT, SLIGHTLY_RIGHT);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, PARTIALLY_RIGHT);
  set_eyeball(EYE_LEFT, PARTIALLY_RIGHT);
  delay(DELAY);
  set_eyeball(EYE_RIGHT, SLIGHTLY_RIGHT);
  set_eyeball(EYE_LEFT, SLIGHTLY_RIGHT);
  delay(DELAY);
}

void expression_3()
{
  set_eyelevel(EYE_RIGHT, LOWER_TILTED);
  set_eyelevel(EYE_LEFT, SLIGHTLY_TILTED_UP);
  set_eyelid(EYE_RIGHT, FULLY_CLOSED);
  set_eyelid(EYE_LEFT, FULLY_CLOSED);
  delay(1000);
  set_eyelid(EYE_RIGHT, PARTIALLY_CLOSED);
  set_eyelid(EYE_LEFT, PARTIALLY_CLOSED);
  delay(1000);
  set_eyelid(EYE_RIGHT, FULLY_CLOSED);
  set_eyelid(EYE_LEFT, FULLY_CLOSED);
  delay(5000);
  set_eyelevel(EYE_RIGHT, RAISED_TILTED);
  set_eyelevel(EYE_LEFT, RAISED_TILTED);
  set_eyelid(EYE_RIGHT, FULLY_OPEN);
  set_eyelid(EYE_LEFT, FULLY_OPEN);
  delay(5000);
}

void animate()
{
  if (animation_start) {
    expression_1();
  }
}

void head_init()
{
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  delay(10);
}