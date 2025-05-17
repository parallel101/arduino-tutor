#include <Servo.h>

const int IRMETER = 12;  // 红外线传感器连到的引脚
const int SERVOL = 11;  // 左舵机连到的引脚
const int SERVOR = 10;  // 右舵机连到的引脚
const int LRBALANCE = 94; // 需要根据左右舵机实际速度情况来调整，调到两边速度一致即可
const int SPEED = 8;  // 舵机运动速度，0到90


Servo servoL;
Servo servoR;

void setup() {
  servoL.attach(SERVOL);
  servoR.attach(SERVOR);
  pinMode(IRMETER, INPUT);
  Serial.begin(115200);
}

void loop() {
  int speedL = SPEED;
  int speedR = SPEED;
  bool obstacled = digitalRead(IRMETER) == 0;
  if (obstacled) {
    speedR = -SPEED;
    speedL = 0;
  }
  servoL.write(LRBALANCE + speedL);
  servoR.write(LRBALANCE - speedR);
  delay(20);
}
