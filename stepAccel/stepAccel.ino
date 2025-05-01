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
const int ABORT = A0;
const int HOLD = A1;
const int RESUME = A2;
const int COOLEN = A3;

float t0 = 0;

void setup() {
  Serial.begin(115200);
  pinMode(XSTP, OUTPUT);
  pinMode(XDIR, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(XDIR, LOW);
  digitalWrite(XSTP, LOW);
  digitalWrite(EN, LOW);
  t0 = micros();
}

int xSignal = 0;

void sendStep(int dir) {
  digitalWrite(XDIR, dir);
  digitalWrite(XSTP, xSignal = !xSignal);
}

float currentPos(float t) {
  return sinf(t);
  // float sign = fmodf(t, 4.0f) - 2.0f;
  // t = fmodf(t, 2.0f) - 1.0f;
  // return max(0.0f, t * t);
}

float currP = 0;
const float STEP = 1.0f;
const float TOL = 8.0f;
const float MAX_ACCEL = 8000.0f;
const float MAX_SPEED = 1000.0f;
const float MIN_SPEED = 80.0f;

void loop() {
  float nextP = map(analogRead(A4), 0, 1023, 0, 1600);
  int dir = 1;
  if (nextP - currP > STEP * TOL) {
    currP += STEP;
    dir = 1;
  } else if (nextP - currP < -STEP * TOL) {
    currP -= STEP;
    dir = 0;
  } else {
    return;
  }
  float dis = abs(nextP - currP);
  float v = max(MIN_SPEED, min(MAX_SPEED, sqrtf(MAX_ACCEL * dis)));
  sendStep(dir);
  delayMicroseconds(long(1000000.0f / v));
}
