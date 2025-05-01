void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  for (int p = 0; p <= 1000; ++p) {
    digitalWrite(13, 1);
    delayMicroseconds(p);
    digitalWrite(13, 0);
    delayMicroseconds(1000 - p);
  }
  for (int p = 1000; p >= 0; --p) {
    digitalWrite(13, 1);
    delayMicroseconds(p);
    digitalWrite(13, 0);
    delayMicroseconds(1000 - p);
  }
}