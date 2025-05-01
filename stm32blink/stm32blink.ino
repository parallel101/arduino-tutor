void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(PC13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(PC13, 1);
  delay(500);
  digitalWrite(PC13, 0);
  delay(500);
  Serial.println("Hello, STM32!");
}
