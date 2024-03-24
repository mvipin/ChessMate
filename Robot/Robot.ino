void setup()
{
  Serial.begin(9600);
  arm_init();
  head_init();
}

void loop() {
  arm_run();
}
