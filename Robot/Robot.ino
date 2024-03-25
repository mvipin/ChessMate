#include "Arm.h"
#include "Head.h"

bool animation_start;

void setup()
{
  Serial.begin(9600);
  arm_init();
  head_init();
}

void loop() {
  arm_run();
}
