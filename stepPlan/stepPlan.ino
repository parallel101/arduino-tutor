const uint8_t XSTP = 2;
const uint8_t YSTP = 3;
const uint8_t ZSTP = 4;
const uint8_t XDIR = 5;
const uint8_t YDIR = 6;
const uint8_t ZDIR = 7;
const uint8_t EN = 8;
const uint8_t XLIM = 9;
const uint8_t YLIM = 10;
const uint8_t ZLIM = 11;
const uint8_t ASTP = 12;
const uint8_t ADIR = 13;
const uint8_t ABORT = A0;
const uint8_t HOLD = A1;
const uint8_t RESUME = A2;
const uint8_t COOLANT = A3;

const uint8_t N_AXIS = 2;
const uint32_t N_SEGMENTS = 1;
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

bool stepNow[N_AXIS];

void enableMotors() {
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    digitalWrite(XDIR + a, HIGH);
  }
  for (uint8_t a = 0; a < N_AXIS; ++a) {
    digitalWrite(XSTP + a, LOW);
    stepNow[a] = 0;
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
  digitalWrite(XSTP + axis, (stepNow[axis] = !stepNow[axis]));
}

void delayFor(float stepTime) {
  delayMicroseconds(unsigned(stepTime * 1000000.0f));
}

void sendSegment(Segment const &segment) {
  uint32_t maxAxisSteps = 0;
  uint8_t maxAxis = N_AXIS;
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
  uint8_t slaveAxis[N_AXIS - 1];
  int32_t slaveDelta[N_AXIS - 1];
  for (uint8_t a = 0; a < N_AXIS - 1; ++a) {
    slaveAxis[a] = (maxAxis + 1 + a) % N_AXIS;
    slaveDelta[a] = 2 * segment.steps[slaveAxis[a]] - maxAxisSteps;
  }
  for (uint32_t i = 0; i < maxAxisSteps; ++i) {
    float speed1Sqr = segment.initSpeedSqr + twiceAcceleration * i;
    float speed2Sqr = segment.exitSpeedSqr + twiceAcceleration * (maxAxisSteps - 1 - i);
    float speedMinSqr = fminf(speed1Sqr, speed2Sqr);
    float stepTime = topStepTime;
    if (speedMinSqr < segment.topSpeedSqr) {
      stepTime = (sqrtf(speedMinSqr + twiceAcceleration) - sqrtf(speedMinSqr)) * invertAcceleration; 
    }
    sendStep(maxAxis);
    for (uint8_t a = 0; a < N_AXIS - 1; ++a) {
      if (slaveDelta[a] > 0) {
        sendStep(slaveAxis[a]);
        slaveDelta[a] -= 2 * maxAxisSteps;
      }
      slaveDelta[a] += 2 * segment.steps[slaveAxis[a]];
    }
    delayFor(stepTime);
  }
}

void setup() {
  Serial.begin(115200);

  segments[0].directions[0] = 1;
  segments[0].steps[0] = 400;
  segments[0].directions[1] = 0;
  segments[0].steps[1] = 4000;
  segments[0].acceleration = 100000;
  segments[0].initSpeedSqr = squaref(100);
  segments[0].topSpeedSqr = squaref(5000);
  segments[0].exitSpeedSqr = squaref(100);

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