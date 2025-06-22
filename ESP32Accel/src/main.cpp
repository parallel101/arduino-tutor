#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_Sensor.h>
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

Adafruit_MPU6050 mpu; // m/s^2 rad/s
Adafruit_HMC5883_Unified hmc; // uT

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    while (!hmc.begin()) {
        printf("Failed to start HMC\n");
        digitalWrite(LED_BUILTIN, ~digitalRead(LED_BUILTIN));
        delay(1000);
    }
    while (!mpu.begin()) {
        printf("Failed to start MPU\n");
        digitalWrite(LED_BUILTIN, ~digitalRead(LED_BUILTIN));
        delay(1000);
    }

    //setupt motion detection
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

    // float gx = 0;
    // float gy = 0;
    // float gz = 0;
    // float ax = 0;
    // float ay = 0;
    // float az = 0;
    // const int num_samples = 500;
    // for (int i = 0; i < num_samples; ++i) {
    //     sensors_event_t a, g, temp;
    //     mpu.getEvent(&a, &g, &temp);
    //     ax += a.acceleration.x;
    //     ay += a.acceleration.y;
    //     gx += g.gyro.x;
    //     gy += g.gyro.y;
    //     gz += g.gyro.z;
    //     delay(17);
    // }
    // calib_ax = ax / num_samples;
    // calib_ay = ay / num_samples;
    // calib_az = az / num_samples;
    // calib_gx = gx / num_samples;
    // calib_gy = gy / num_samples;
    // calib_gz = gz / num_samples;
    // printf("calibration ax=%f ay=%f az=%f gx=%f gy=%f gz=%f\n",
    //        calib_ax, calib_ay, calib_az,
    //        calib_gx, calib_gy, calib_gz);
    // delay(200);
}

float roll_prev = 0;
float pitch_prev = 0;
float yaw_prev = 0;
float t_prev = 0;
const float alpha = 0.98;

void loop()
{
    sensors_event_t a, g, temp, m;
    mpu.getEvent(&a, &g, &temp);
    hmc.getEvent(&m);

    float t = a.timestamp * 0.001;
    float ax = ka.lowPass(kax, a.acceleration.x);
    float ay = ka.lowPass(kay, a.acceleration.y);
    float az = ka.lowPass(kaz, a.acceleration.z);
    float gx = kg.lowPass(kgx, g.gyro.x);
    float gy = kg.lowPass(kgy, g.gyro.y);
    float gz = kg.lowPass(kgz, g.gyro.z);
    float mx = km.lowPass(kmx, m.magnetic.x);
    float my = km.lowPass(kmy, m.magnetic.y);
    float mz = km.lowPass(kmz, m.magnetic.z);
    mx += 46.0f;
    my += 11.5f;
    mz += -26.0f;

    // float tmp = ay;
    // ay = -ax;
    // ax = tmp;
    // tmp = gy;
    // gy = -gx;
    // gx = tmp;
    // mx = -mx;
    // my = -my;

    float dt = t_prev >= t || t_prev == 0 ? 0.0f : t - t_prev;
    float roll = atan2(ay, az);
    float pitch = atan2(-ax, sqrt(ay * ay + az * az));
    float yaw = atan2(my, mx);
    roll_prev = roll;
    pitch_prev = pitch;
    yaw_prev = yaw;
    t_prev = t;

    printf("gx=%f gy=%f gz=%f ax=%f ay=%f az=%f mx=%f my=%f mz=%f roll=%f pitch=%f yaw=%f\n", gx, gy, gz, ax, ay, az, mx, my, mz, degrees(roll), degrees(pitch), degrees(yaw));

    delay(27);
}
