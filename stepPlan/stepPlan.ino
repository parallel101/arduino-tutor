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

const int N_AXIS = 2;
const int N_SEGMENTS = 1;
const float STEP_SIZE = 0.05f; // mm

struct Segment {
  uint8_t directions[N_AXIS];
  uint32_t steps[N_AXIS];
  float initSpeedSqr; // (step/s)^2
  float topSpeedSqr;
  float exitSpeedSqr;
  float acceleration; // step/s^2
};

Segment segments[N_SEGMENTS];

void initMotors() {
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    pinMode(XSTP + a, OUTPUT);
    pinMode(XDIR + a, OUTPUT);
  }
}

void enableMotors() {
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    digitalWrite(XDIR + a, HIGH);
  }
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    digitalWrite(XSTP + a, LOW);
  }
  digitalWrite(EN, LOW);
}

void disableMotors() {
  digitalWrite(EN, HIGH);
}

void setDirection(uint8_t axis, bool direction) {
  digitalWrite(XDIR + axis, direction);
}

void sendStep(uint8_t axis) {
  digitalWrite(XSTP + axis, !digitalRead(XSTP));
}

void delayFor(float stepTime) {
  delayMicroseconds(unsigned(stepTime * 1000000.0f));
}

void sendSegment(Segment const &segment) {
  uint32_t maxAxisSteps = 0;
  uint32_t maxAxis = N_AXIS;
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    setDirection(a, segment.directions[a]);
    if (segment.steps[a] > maxAxisSteps) {
      maxAxisSteps = segment.steps[a];
      maxAxis = a;
    }
  }
  if (maxAxisSteps == 0) {
    return;
  }

  float topStepTime = 1.0f / sqrtf(segment.topSpeedSqr);
  float invertAcceleration =  1.0f / segment.acceleration;
  float twiceAcceleration = 2.0f * segment.acceleration;
  int slaveAxis = 1 - maxAxis;
  int slaveDelta = 2 * segment.steps[slaveAxis] - maxAxisSteps;
  for (uint32_t i = 0; i < maxAxisSteps; ++i) {
    float speed1Sqr = segment.initSpeedSqr + twiceAcceleration * i;
    float speed2Sqr = segment.exitSpeedSqr + twiceAcceleration * (maxAxisSteps - 1 - i);
    float speedMinSqr = fminf(speed1Sqr, speed2Sqr);
    float stepTime;
    if (speedMinSqr >= segment.topSpeedSqr) {
      stepTime = topStepTime;
    } else {
      stepTime = (sqrtf(speedMinSqr + twiceAcceleration) - sqrtf(speedMinSqr)) * invertAcceleration; 
    }
    sendStep(maxAxis);
    if (slaveDelta > 0) {
      sendStep(slaveAxis);
      slaveDelta -= 2 * maxAxisSteps;
    }
    slaveDelta += 2 * segment.steps[slaveAxis];
    delayFor(stepTime);
  }
}

void setup() {
  Serial.begin(115200);

  segments[0].directions[0] = 1;
  segments[0].steps[0] = 1000;
  segments[0].directions[1] = 0;
  segments[0].steps[1] = 300;
  segments[0].acceleration = 4000;
  segments[0].initSpeedSqr = squaref(10);
  segments[0].topSpeedSqr = squaref(1000);
  segments[0].exitSpeedSqr = squaref(10);

  initMotors();
  enableMotors();
}

void loop() {
  sendSegment(segments[0]);
  delay(100);
}
// s=v0t+0.5at^2
// 0.5a t^2 + v0 t - s = 0
// delta = v0^2 + 2as
// t = (-v0 +- sqrt delta) / a
// t = (sqrt(v0^2 + 2as) - v0) / a
// v = sqrt(v0^2 + 2as)