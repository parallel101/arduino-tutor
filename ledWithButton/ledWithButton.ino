void setup() {
  pinMode(13, OUTPUT);
  pinMode(7, INPUT);
}

void loop() {
  int value = digitalRead(7);
  value = !value;
  digitalWrite(13, value);
}