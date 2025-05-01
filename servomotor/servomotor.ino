#include <Servo.h>

Servo servo;

void setup() {
  Serial.begin(9600);
  servo.attach(6);
}

void loop() {
  int value = analogRead(5);
  value = map(value, 0, 1023, 0, 180);
  Serial.println(value);
  servo.write(value);
  delay(15);
}
