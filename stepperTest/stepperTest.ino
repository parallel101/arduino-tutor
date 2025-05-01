#include <Arduino.h>
#include <TM1637Display.h>
#include <Stepper.h>

#define LEDR 9
#define LEDG 10
#define LEDB 11
#define IRIN 8
#define VRX A1
#define VRY A2
#define VBTN 12
#define STEP_PER_REVO 2048

Stepper stepper(STEP_PER_REVO, 4, 6, 5, 7);

void setup() {
  Serial.begin(9600);
  stepper.setSpeed(6);
}

void loop() {
  stepper.step(-5);
}
