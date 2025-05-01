#include <Arduino.h>

#define VRX 1
#define VRY 0
#define SW 7
#define LED 13
#define IA 10
#define IB 11


void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  pinMode(SW, INPUT);
}

void loop() {
  int x = analogRead(VRX);
  int y = analogRead(VRY);
  x = map(x, 0, 1023, 0, 180);
  y = map(y, 0, 1023, -255, 255);
  Serial.print(x);
  Serial.print(' ');
  Serial.print(y);
  Serial.print('\n');

  if (abs(y) < 20) {
    y = 0;
  }
  analogWrite(IA, max(0, y));
  analogWrite(IB, max(0, -y));
}
