void setup() {
  pinMode(12, OUTPUT);
  Serial.begin(115200);
}

void setAngle(int angle) {
  // angle: 0-180
  // us: 500-2500
  // int us = 500 + 2000 / 180 * angle;
  int us = map(angle, 0, 180, 500, 2500);
  digitalWrite(12, 1);
  delayMicroseconds(us);
  digitalWrite(12, 0);
  delayMicroseconds(20000 - us);
}

void loop() {
  int value = analogRead(A0);
  int angle = map(value, 0, 1023, 0, 180);
  setAngle(angle);
}
