#include <Servo.h>

Servo servoX;
Servo servoY;

void setup() {
  servoX.attach(12);
  servoY.attach(11);
  Serial.begin(115200);
}

void loop() {
  int x = analogRead(A0);
  int y = analogRead(A1);
  int ax = map(x, 0, 1023, 0, 180);
  int ay = map(y, 0, 1023, 0, 180);
  servoX.write(ax);
  servoY.write(ay);
  delay(15);
}
