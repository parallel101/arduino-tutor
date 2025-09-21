#include <Arduino.h>
#include <basicMPU6050.h>
#include <imuFilter.h>
#include <Wire.h>

basicMPU6050<> mpu;
imuFilter fusion;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN_AUX, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN_AUX, HIGH);

    mpu.setup();
    mpu.setBias();
    fusion.setup(mpu.ax(), mpu.ay(), mpu.az());
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_BUILTIN_AUX, LOW);
}

void loop()
{
    delay(27);

    mpu.updateBias();
    fusion.update(mpu.gx(), mpu.gy(), mpu.gz(), mpu.ax(), mpu.ay(), mpu.az());

    quat_t q = fusion.getQuat();
    printf("qw=%f qx=%f qy=%f qz=%f\n", q.get(0), q.get(1), q.get(2), q.get(3));
}
