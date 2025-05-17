#include <Servo.h>

Servo servoL;
Servo servoR;

void setup() {
  servoL.attach(10);
  servoR.attach(11);
  pinMode(4, INPUT);
  Serial.begin(115200);
}

void loop() {
  int speedL = 5;
  int speedR = 5;
  bool obstacled = digitalRead(4) == 0;
  if (obstacled) {
    speedR = -5;
  }
  servoL.write(90 + speedL);
  servoR.write(90 - speedR);
  delay(20);
}
