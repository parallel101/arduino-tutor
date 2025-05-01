void setup() {
  // put your setup code here, to run once:
  pinMode(8, INPUT);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  Serial.begin(9600);
}

void loop() {
  int enable = digitalRead(8);
  if (enable == 0) {
    int hz = 800 + 500     * sin(millis() * 0.0005);
    // Serial.println(hz);
    int step = 1000000 / hz;
    digitalWrite(7, LOW);
    delayMicroseconds(step);
    digitalWrite(7, HIGH);
    delayMicroseconds(step);
  } else {
    digitalWrite(7, HIGH);
  }
}
