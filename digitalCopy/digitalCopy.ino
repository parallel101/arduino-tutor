#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  int value = analogRead(5);
  value = map(value, 0, 1023, -255, 255);
  Serial.println(value);
  if (abs(value) < 20) {
    value = 0;
  }
  analogWrite(10, max(0, -value));
  analogWrite(11, max(0, value));
  digitalWrite(13, value != 0);
}
