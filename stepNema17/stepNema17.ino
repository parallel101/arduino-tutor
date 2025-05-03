const int XSTP = 2;
const int YSTP = 3;
const int ZSTP = 4;
const int XDIR = 5;
const int YDIR = 6;
const int ZDIR = 7;
const int EN = 8;
const int XLIM = 9;
const int YLIM = 10;
const int ZLIM = 11;
const int ASTP = 12;
const int ADIR = 13;
const int ABORT = A0;
const int HOLD = A1;
const int RESUME = A2;
const int COOLANT = A3;


void setup() {
  Serial.begin(115200);
  pinMode(XSTP, OUTPUT);
  pinMode(XDIR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);
}

const int STEPS_PER_REVO = 200 * 4;

void loop() {
  digitalWrite(XDIR, HIGH);
  for (int x = 0; x < 800; ++x) {
    digitalWrite(XSTP, HIGH);
    delayMicroseconds(200);
    digitalWrite(XSTP, LOW);
    delayMicroseconds(200); 
  }
  delay(1000);
  digitalWrite(XDIR, LOW);
  for (int x = 0; x < 800; ++x) {
    digitalWrite(XSTP, HIGH);
    delayMicroseconds(200);
    digitalWrite(XSTP, LOW);
    delayMicroseconds(200); 
  }
  delay(1000);
}
