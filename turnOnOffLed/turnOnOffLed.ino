void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(Serial.available()>0)
  {
    char ch=Serial.read();
    if (ch == 'a') {
      digitalWrite(13, HIGH);
      Serial.println("ON");
    } else {
      digitalWrite(13, LOW);
      Serial.println("OFF");
    }
  }
  delay(100);
}
