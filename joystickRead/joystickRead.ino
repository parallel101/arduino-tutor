void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int x = analogRead(2) - 512;
  int y = analogRead(1) - 512;
  if (abs(x) < 10) x = 0;
  if (abs(y) < 10) y = 0;
  Serial.print("x=");
  Serial.print(x, DEC);
  Serial.print(", y=");
  Serial.print(y, DEC);
  Serial.print("\n");
  analogWrite(9, abs(x) / 4);
  analogWrite(11, abs(y) / 4);
}
