class Servo {
  static const int kMaxPins = 10;
  static const int kMinValue = 500;
  static const int kMaxValue = 2500;
  static const int kInterval = 20000;
  static const int kOutputDelay = 10;

  int pins[kMaxPins]{};
  int values[kMaxPins]{};
  int numPins{};
  int updateTime{};

public:
  void attachAt(int index, int pin) {
    if (numPins <= index) {
      numPins = index + 1;
    }
    pins[index] = pin;
    values[index] = 0;
    pinMode(pin, OUTPUT);
  }

  void attach(int pin) {
    int index = numPins++;
    attachAt(index, pin);
  }

  int attachCount() const {
    return numPins;
  }

  int indexToPin(int index) const {
    return pins[index];
  }

  int pinToIndex(int pin) const {
    for (int i = 0; i < numPins; ++i) {
      if (pins[i] == pin) {
        return i;
      }
    }
    return -1;
  }

  void update() {
    int now = micros();
    int dt = updateTime - now;
    if (dt > 0) {
      delayMicroseconds(dt);
    }
    now = micros();
    for (int i = 0; i < numPins; ++i) {
      if (kMinValue <= values[i] && values[i] <= kMaxValue) {
        digitalWrite(pins[i], HIGH);
        delayMicroseconds(values[i] - kOutputDelay);
        digitalWrite(pins[i], LOW);
        delayMicroseconds(kMaxValue - values[i] - kOutputDelay);
      } else {
        digitalWrite(pins[i], LOW);
        delayMicroseconds(kMaxValue - kOutputDelay);
      }
    }
    updateTime = now + kInterval;
  }

  void setMicroseconds(int index, int value) {
    values[index] = value;
  }

  void setAngle(int index, int angle, int minAngle, int maxAngle) {
    values[index] = map(angle, minAngle, maxAngle, kMinValue, kMaxValue);
  }

  void setAngle(int index, int angle) {
    values[index] = map(angle, 0, 180, kMinValue, kMaxValue);
  }

  void setFree(int index) {
    values[index] = 0;
  }
};

Servo servo;

void setup() {
  Serial.begin(9600);
  pinMode(12, INPUT);
  servo.attach(8);
  servo.attach(9);
  servo.attach(11);
  pinMode(10, OUTPUT);
}

void loop() {
  int value = analogRead(5);
  servo.setAngle(0, value, 0, 1023);
  servo.setAngle(1, 1023 - value, 0, 1023);
  servo.update();
  Serial.println(value);
}
