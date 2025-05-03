void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 256kHz - 4Hz
  OCR1A = 1024;
  TIMSK1 = _BV(OCIE1A);
  DDRD |= B11111100;
  DDRB |= B00110000;
}

#define SA 0
#define DA 1
#define SX 2
#define SY 3
#define SZ 4
#define DX 5
#define DY 6
#define DZ 7
#define TCK 8

volatile uin16_t step = 1024;

void loop() {
  uint16_t v = map(analogRead(A0), 0, 1023, 2, 65535);
  t = v;
  Serial.println(v);
  delay(100);
}

ISR(TIMER1_COMPA_vect) {
  OCR1A = step >> TCK;
  PORTD = (step & B11111100) | (PORTD & B00000011);
  PORTB = ((step & B00000011) << PB4) | (PORTB & B11001111);
}