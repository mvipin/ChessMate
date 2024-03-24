#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

enum {
  SERVO_RIGHT_EYELID,
  SERVO_RIGHT_EYEBALL,
  SERVO_RIGHT_HEAD,
  SERVO_LEFT_EYELID,
  SERVO_LEFT_EYEBALL,
  SERVO_LEFT_HEAD,
  SERVO_NUM_MAX,
};

typedef struct {
  uint16_t pulse_min;
  uint16_t pulse_max;
  uint16_t degree_low;
  uint16_t degree_high;
} servo_cfg_t;

servo_cfg_t servo_cfg[SERVO_NUM_MAX] = {
  {80, 530, 35, 100}, // SERVO_RIGHT_EYELID (0)
  {80, 530, 40, 120}, // SERVO_RIGHT_EYEBALL (1)
  {100, 500, 0, 180}, // SERVO_RIGHT_HEAD (2)
  {80, 530, 80, 135}, // SERVO_LEFT_EYELID (3)
  {80, 530, 55, 140}, // SERVO_LEFT_EYEBALL (4)
  {100, 500, 0, 180}, // SERVO_LEFT_HEAD (5)
};

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

void set_eyeball(bool right, bool left)
{
  uint16_t pulselen;

  if (right) {
    if (left) {
      pulselen = map(servo_cfg[SERVO_RIGHT_EYEBALL].degree_high, 0, 180, servo_cfg[SERVO_RIGHT_EYEBALL].pulse_min, servo_cfg[SERVO_RIGHT_EYEBALL].pulse_max);
    } else {
      pulselen = map(servo_cfg[SERVO_RIGHT_EYEBALL].degree_low, 0, 180, servo_cfg[SERVO_RIGHT_EYEBALL].pulse_min, servo_cfg[SERVO_RIGHT_EYEBALL].pulse_max);
    }
    pwm.setPWM(SERVO_RIGHT_EYEBALL, 0, pulselen);
  } else {
    if (left) {
      pulselen = map(servo_cfg[SERVO_LEFT_EYEBALL].degree_low, 0, 180, servo_cfg[SERVO_LEFT_EYEBALL].pulse_min, servo_cfg[SERVO_LEFT_EYEBALL].pulse_max);
    } else {
      pulselen = map(servo_cfg[SERVO_LEFT_EYEBALL].degree_high, 0, 180, servo_cfg[SERVO_LEFT_EYEBALL].pulse_min, servo_cfg[SERVO_LEFT_EYEBALL].pulse_max);
    }
    pwm.setPWM(SERVO_LEFT_EYEBALL, 0, pulselen);
  }
}

void set_eyelid(bool right, bool open)
{
  uint16_t pulselen;

  if (right) {
    if (open) {
      pulselen = map(servo_cfg[SERVO_RIGHT_EYELID].degree_high, 0, 180, servo_cfg[SERVO_RIGHT_EYELID].pulse_min, servo_cfg[SERVO_RIGHT_EYELID].pulse_max);
    } else {
      pulselen = map(servo_cfg[SERVO_RIGHT_EYELID].degree_low, 0, 180, servo_cfg[SERVO_RIGHT_EYELID].pulse_min, servo_cfg[SERVO_RIGHT_EYELID].pulse_max);
    }
    pwm.setPWM(SERVO_RIGHT_EYELID, 0, pulselen);
  } else {
    if (open) {
      pulselen = map(servo_cfg[SERVO_LEFT_EYELID].degree_high, 0, 180, servo_cfg[SERVO_LEFT_EYELID].pulse_min, servo_cfg[SERVO_LEFT_EYELID].pulse_max);
    } else {
      pulselen = map(servo_cfg[SERVO_LEFT_EYELID].degree_low, 0, 180, servo_cfg[SERVO_LEFT_EYELID].pulse_min, servo_cfg[SERVO_LEFT_EYELID].pulse_max);
    }
    pwm.setPWM(SERVO_LEFT_EYELID, 0, pulselen);
  }
}

void head_test()
{
  static uint8_t servonum = SERVO_RIGHT_EYELID;
  uint16_t pulselen;

  // Drive each servo one at a time using setPWM()
  Serial.println(servonum);
#if 1
  for (uint8_t degrees = servo_cfg[servonum].degree_low; degrees < servo_cfg[servonum].degree_high; degrees++) {
    pulselen = map(degrees, 0, 180, servo_cfg[servonum].pulse_min, servo_cfg[servonum].pulse_max);
    pwm.setPWM(servonum, 0, pulselen);
    delay(50);
  }
#else
    pulselen = map(servo_cfg[servonum].degree_high, 0, 180, servo_cfg[servonum].pulse_min, servo_cfg[servonum].pulse_max);
    pwm.setPWM(servonum, 0, pulselen);
    delay(1000);
#endif

#if 1
  for (uint16_t degrees = servo_cfg[servonum].degree_high; degrees > servo_cfg[servonum].degree_low; degrees--) {
    pulselen = map(degrees, 0, 180, servo_cfg[servonum].pulse_min, servo_cfg[servonum].pulse_max);
    pwm.setPWM(servonum, 0, pulselen);
    delay(50);
  }
#else
    pulselen = map(servo_cfg[servonum].degree_low, 0, 180, servo_cfg[servonum].pulse_min, servo_cfg[servonum].pulse_max);
    pwm.setPWM(servonum, 0, pulselen);
    delay(1000);
#endif

  servonum++;
  if (servonum >= SERVO_NUM_MAX) servonum = 0;
}

void head_init()
{
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  delay(10);
}