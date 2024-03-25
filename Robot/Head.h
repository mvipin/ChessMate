#ifndef HEAD_H
#define HEAD_H

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

typedef enum {
  LEFT,
  RIGHT,
} dir_t;

typedef enum {
  SERVO_RIGHT_EYELID,
  SERVO_RIGHT_EYEBALL,
  SERVO_RIGHT_EYELEVEL,
  SERVO_LEFT_EYELID,
  SERVO_LEFT_EYEBALL,
  SERVO_LEFT_EYELEVEL,
  SERVO_NUM_MAX,
} servo_id_t;

typedef enum {
  POS_3H,
  POS_2H,
  POS_1H,
  POS_12H,
  POS_11H,
  POS_10H,
  POS_9H,
} pos_clk_t;

typedef enum {
  FULLY_OPEN,
  PARTIALLY_OPEN,
  HALF_OPEN,
  PARTIALLY_CLOSED,
  FULLY_CLOSED,
} pos_gap_t;

typedef enum {
  FULLY_DOWN,
  LOWER_TILTED,
  SLIGHTLY_TILTED_DOWN,
  NEUTRAL,
  SLIGHTLY_TILTED_UP,
  RAISED_TILTED,
  FULLY_UP,
} pos_height_t;

typedef struct {
  uint16_t pulse_min;
  uint16_t pulse_max;
  uint16_t degree_low;
  uint16_t degree_high;
} servo_cfg_t;

#endif // HEAD_H