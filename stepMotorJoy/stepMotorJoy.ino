#include <Arduino.h>
#include <TM1637Display.h>
#include <Stepper.h>

class Joystick
{
public:
  explicit Joystick(int vrx, int vry, int btn, int tolerance = 0)
  : vrx(vrx), vry(vry), btn(btn), lastX(0), lastY(0), tolerance(tolerance)
  {
    pinMode(btn, INPUT);
  }

  int button() const
  {
    return digitalRead(btn);
  }

  int x() const
  {
    return intRead(analogFetch(vrx, &lastX));
  }

  int y() const
  {
    return intRead(analogFetch(vry, &lastY));
  }

  float fx() const
  {
    return floatRead(analogFetch(vrx, &lastX));
  }

  float fy() const
  {
    return floatRead(analogFetch(vry, &lastY));
  }

private:
  int analogFetch(int pin, int *last) const
  {
    int value = analogRead(pin);
    if (abs(value - *last) < tolerance) {
      return *last;
    }
    *last = value;
    return value;
  }

  float floatRead(int value) const
  {
    if (abs(value - 512) <= tolerance) {
      value = 512;
    }
    return (value - 512) * (1.0f / 512.0f);
  }

  int intRead(int value) const
  {
    if (abs(value - 512) <= tolerance) {
      value = 512;
    }
    return value - 512;
  }

  int vrx, vry, btn;
  mutable int lastX, lastY;
  int tolerance;
};

class Motion
{
public:
  explicit Motion(int minStep = -1, int maxStep = -1)
  : minStep(minStep), maxStep(maxStep), pos(0) {}

  int position() const
  {
    return pos;
  }

  int moveTo(int p)
  {
    int step = p - pos;
    if (maxStep > 0) {
      step = constrain(step, -maxStep, maxStep);
    }
    pos += step;
    int absStep = abs(step);
    if (minStep > 0 && absStep < minStep) {
      step = 0;
    }
    return step;
  }

private:
  int minStep;
  int maxStep;
  int pos;
};

#define LEDR 9
#define LEDG 10
#define LEDB 11
#define IRIN 8
#define VRX A1
#define VRY A2
#define VBTN 12
#define STEP_PER_REVO 2048

Stepper stepper(STEP_PER_REVO, 7, 5, 6, 4);
Joystick joystick(VRX, VRY, VBTN, 4);
Motion motion(-1, 20);

void setup() {
  Serial.begin(9600);
  pinMode(VRX, INPUT);
  pinMode(VRY, INPUT);
  pinMode(VBTN, INPUT);
  pinMode(IRIN, INPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  stepper.setSpeed(6);
}

void loop() {
  int value = joystick.x();
  value = map(value, -512, 512, -STEP_PER_REVO / 2, STEP_PER_REVO / 2);
  int step = motion.moveTo(value);
  Serial.print(value);
  Serial.print(' ');
  Serial.print(step);
  Serial.println();
  stepper.step(step);
}
