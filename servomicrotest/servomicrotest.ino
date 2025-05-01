void setup() {
  pinMode(3, OUTPUT);
}

void loop() {
  digitalWrite(3, HIGH);    // sets the servo pin on
  delayMicroseconds(1200);         // pauses for 1465 microseconds
  digitalWrite(3, LOW);     // sets the servo pin off
  delayMicroseconds(20000 - 1200); // pauses for 18535 microseconds
}
