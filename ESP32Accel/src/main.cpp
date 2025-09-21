#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_Sensor.h>
#include <VectorXf.h>
#include <Wire.h>

struct Kalman
{
private:
    const float K0;
    const float R;
    const float Q = (K0 * K0 * R) / (1 - K0);

public:
    struct State {
        float P = 0;
        float x_low = 0;
    };

    explicit Kalman(float fs, float fc, float R = 1.0f)
        : K0(1 - exp((-2 * (float)M_PI) * (fc / fs)))
        , R(R)
    {
        assert(fc < 0.5f * fs);
    }

    void init(State &state, float z0 = 0.0f) {
        state.P = K0 * R;
        state.x_low = z0;
    }

    float highPass(State &state, float z) {
        state.P += Q;
        float K = state.P / (state.P + R);
        float r = z - state.x_low;
        state.x_low += K * r;
        state.P *= 1 - K;
        float y = z - state.x_low;
        return y;
    }

    float lowPass(State &state, float z) {
        state.P += Q;
        float K = state.P / (state.P + R);
        float r = z - state.x_low;
        state.x_low += K * r;
        state.P *= 1 - K;
        return state.x_low;
    }

    float allPass(State &state, float z) {
        (void)state;
        return z;
    }
};

Kalman ka(100.0f, 3.0f, 0.1f);
Kalman kg(100.0f, 15.0f, 0.005f);
Kalman km(100.0f, 3.0f, 1.0f);
Kalman::State kax, kay, kaz;
Kalman::State kgx, kgy, kgz;
Kalman::State kmx, kmy, kmz;

const float mcalib[3] = {-30.513301f, -5.6385325f, 61.5121215f};

Adafruit_MPU6050 mpu;
Adafruit_HMC5883_Unified hmc;

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

    ka.init(kax);
    ka.init(kay);
    ka.init(kaz);
    kg.init(kgx);
    kg.init(kgy);
    kg.init(kgz);
}

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

    printf("t=%f gx=%f gy=%f gz=%f ax=%f ay=%f az=%f mx=%f my=%f mz=%f\n", t, gx, gy, gz, ax, ay, az, mx, my, mz);

    // Vec3f accel(ax, ay, az);
    // Vec3f magnet(mx, my, mz);
    // Vec3f gyro(gx, gy, gz);
    //
    // accel.normalize();
    // magnet -= magnet.dot(accel) * accel;
    // magnet.normalize();
    //
    // Vec3f left = magnet.getCrossed(accel);
    // left.normalize();
    // magnet = accel.getCrossed(left);

    delay(27);
}
