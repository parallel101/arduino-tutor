#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_Sensor.h>
#include <imuFilter.h>
#include <Wire.h>

// Kalman ka(100.0f, 3.0f, 0.1f);
// Kalman kg(100.0f, 15.0f, 0.005f);
// Kalman km(100.0f, 3.0f, 1.0f);
// Kalman::State kax, kay, kaz;
// Kalman::State kgx, kgy, kgz;
// Kalman::State kmx, kmy, kmz;

const float mcalib[3] = {-30.513301f, -5.6385325f, 61.5121215f};
const float gcalib[3] = {-0.0070133349056603735f, 0.05005577873070325f, -0.014073409519725605f};

Adafruit_MPU6050 mpu;
Adafruit_HMC5883_Unified hmc;
imuFilter imu;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    while (!hmc.begin()) {
        printf("Failed to start HMC\n");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
    }
    while (!mpu.begin()) {
        printf("Failed to start MPU\n");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
    }

    hmc.setMagGain(HMC5883_MAGGAIN_1_3);
    mpu.setClock(MPU6050_INTR_8MHz);
    mpu.setCycleRate(MPU6050_CYCLE_40_HZ);
    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true);
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);

    delay(100);
    imu.setup();

    // ka.init(kax);
    // ka.init(kay);
    // ka.init(kaz);
    // kg.init(kgx);
    // kg.init(kgy);
    // kg.init(kgz);
    // km.init(kmx);
    // km.init(kmy);
    // km.init(kmz);
}

// float prev_t = 0;
// quat curr_ori{true};

void loop()
{
    sensors_event_t a, g, temp, m;
    mpu.getEvent(&a, &g, &temp);
    hmc.getEvent(&m);

    float t = a.timestamp * 0.001;
    // m/s^2
    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    // rad/s
    float gx = g.gyro.x;
    float gy = g.gyro.y;
    float gz = g.gyro.z;
    // uT
    float mx = m.magnetic.x;
    float my = m.magnetic.y;
    float mz = m.magnetic.z;

    float tmp = ay;
    ay = -ax;
    ax = tmp;
    tmp = gy;
    gy = -gx;
    gx = tmp;
    mx -= mcalib[0];
    my -= mcalib[1];
    mz -= mcalib[2];
    gx -= gcalib[0];
    gy -= gcalib[1];
    gz -= gcalib[2];

    printf("t=%f gx=%f gy=%f gz=%f ax=%f ay=%f az=%f mx=%f my=%f mz=%f\n", t, gx, gy, gz, ax, ay, az, mx, my, mz);

    imu.update(gx, gy, gz, ax, ay, az);

    quat_t q = imu.getQuat();
    printf("t=%f qw=%f qx=%f qy=%f qz=%f\n", t, q.get(0), q.get(1), q.get(2), q.get(3));

    // float dt = t - prev_t;
    // prev_t = t;

    // vector3 accel(ax, ay, az);
    // vector3 magnet(mx, my, mz);
    // vector3 gyro(gx, gy, gz);
    //
    // accel.normalize();
    // magnet.normalize();
    // vector3 left = magnet;
    // left.cross(accel);
    // left.normalize();
    // magnet = accel;
    // magnet.cross(left);
    //
    // matrix4 rot_matrix;
    // rot_matrix[0] = vector4(left.x, left.y, left.z, 0.0f);
    // rot_matrix[1] = vector4(magnet.x, magnet.y, magnet.z, 0.0f);
    // rot_matrix[2] = vector4(accel.x, accel.y, accel.z, 0.0f);
    // rot_matrix[3] = vector4(0.0f, 0.0f, 0.0f, 1.0f);
    // quat ori;
    // ori.fromOrthonormalMatrix(rot_matrix);
    //
    // curr_ori.integrate(gyro, dt);
    // quat::slerp(curr_ori, curr_ori, ori, 0.03);
    // curr_ori.normalize();
    //
    // printf("t=%f qx=%f qy=%f qz=%f qw=%f\n", t, curr_ori.X(), curr_ori.Y(), curr_ori.Z(), curr_ori.A());

    delay(27);
}
