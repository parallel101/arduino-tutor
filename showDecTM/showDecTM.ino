#include <Arduino.h>
#include <TimerOne.h>
#include <TM1637Display.h>

#define DIO 4
#define CLK 5

TM1637Display tm(CLK, DIO);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int t = analogRead(4);
  Serial.println(t);
  tm.setBrightness(2, true);
  tm.showNumberDecEx(t, 0b01000000, true);
  delay(100);
}
