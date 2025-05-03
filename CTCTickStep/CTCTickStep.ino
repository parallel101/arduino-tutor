namespace CNC {


struct alignas(uint16_t) Step {
  uint16_t ticks : 13;
  uint8_t axis : 2;
  uint8_t dir : 1;
};

Step ring[256];
uint8_t wr;
uint8_t rd;

enum ErrorBits {
  EUNDERFLOW,
  EOVERFLOW,
};

uint8_t error;
uint32_t tickPerSec;
float mmPerRevo = 40;
float stepPerRevo = 800;

enum TickRate : uint32_t {
  Tick_2MHz = 2000000,
  Tick_250kHz = 250000,
  Tick_62500Hz = 62500,
  Tick_15625Hz = 15625,
};

void begin(TickRate tickRate = Tick_62500Hz) {
  cli();
  uint8_t cs = 0;
  switch (tickRate) {
    case Tick_2MHz: cs = _BV(CS11); break; // 2MHz - 8kHz
    case Tick_250kHz: cs = _BV(CS11) | _BV(CS10); break; // 250kHz - 977Hz
    case Tick_62500Hz: cs = _BV(CS12); break; // 62.5kHz - 244Hz
    case Tick_15625Hz: cs = _BV(CS12) | _BV(CS10); break; // 15625Hz - 61Hz
    default: cs = 0; break;
  };
  tickPerSec = tickRate;
  
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | cs; 
  OCR1A = 255;
  TIMSK1 = _BV(OCIE1A);
  PORTD &= B00000011;
  PORTB &= B11001111;
  PORTB |= B00000001;
  DDRD |= B11111100;
  DDRB |= B00110001;
  wr = 0;
  rd = 0;
  error = 0;
  sei();
}

void enable(bool on) {
  if (on) {
    PORTB &= ~B00000001;
  } else {
    PORTB |= B00000001;
  }
}

bool ringw(Step const &step) {
  cli();
  uint8_t p = wr;
  if (p + 1 == rd) {
    sei();
    return false;
  }
  ring[p] = step;
  wr = p + 1;
  sei();
  return true;
}

bool ringr(Step &step) {
  uint8_t p = rd;
  if (p == wr) {
    error |= _BV(EUNDERFLOW);
    return false;
  }
  step = ring[p];
  rd = p + 1;
  return true;
}

void ISR_TIMER1_COMPA_vect() {
  cli();
  Step step;
  if (!ringr(step)) {
    sei();
    return;
  }
  OCR1A = step.ticks;
  switch (step.axis) {
  case 0:
    bitWrite(PORTD, PD5, step.dir);
    bitToggle(PORTD, PD2);
    break;
  case 1:
    bitWrite(PORTD, PD6, step.dir);
    bitToggle(PORTD, PD3);
    break;
  case 2:
    bitWrite(PORTD, PD7, step.dir);
    bitToggle(PORTD, PD4);
    break;
  case 3:
    bitWrite(PORTB, PB5, step.dir);
    bitToggle(PORTB, PB4);
    break;
  }
  sei();
}

void send(uint8_t axis, uint8_t dir, uint16_t ticks) {
  Step step;
  step.ticks = ticks;
  step.axis = axis;
  step.dir = dir;
  ringw(step);
}

uint8_t lastError() {
  cli();
  uint8_t e = error;
  error = 0;
  sei();
  return e;
}

uint16_t mmPerSec_to_tickPerStep(float mmPerSec) {
  float mmPerStep = mmPerRevo / stepPerRevo;
  float stepPerSec = mmPerSec / mmPerStep;
  float tickPerStep = tickPerSec / stepPerSec;
  return uint16_t(tickPerStep) - 1;
}

}

ISR(TIMER1_COMPA_vect) {
  CNC::ISR_TIMER1_COMPA_vect();
}

void setup() {
  Serial.begin(9600);
  CNC::begin();
  CNC::enable(true);
}

void loop() {
  long mmps = map(analogRead(SDA), 0, 1023, 10, 200);
  uint16_t tick = CNC::mmPerSec_to_tickPerStep(mmps);
  CNC::send(0, 1, tick);
  uint8_t error = CNC::lastError();
  if (error) {
    CNC::enable(false);
    Serial.print("ERROR:");
    Serial.print(error, BIN);
    Serial.print(" SPEED:");
    Serial.print(mmps);
    Serial.print(" TICKS:");
    Serial.println(tick);
    delay(1000);
    CNC::begin();
    CNC::enable(true);
  }
}