#include "Arm.h"
#include "Head.h"

animation_id_t animation_id;

void setup()
{
  Serial.begin(9600);
  head_init();
  arm_init();
}

void loop() {
  arm_run();
  animate();
}
