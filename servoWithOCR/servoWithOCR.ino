void setup() {
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(COM1B1);
  TCCR1B = _BV(WGM13) | _BV(CS11);
  ICR1 = 40000;
  TIMSK1 = _BV(TOIE1);
  sei();
}

volatile bool ov = 0;

void loop() {
  if (ov) {
    ov = 0;
    Serial.println("overflow");
  }
  OCR1B = map(analogRead(A0), 0, 1023, 500, 2500);
}

ISR(TIMER1_OVF_vect) {
  ov = 1;
}