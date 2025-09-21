#include <Arduino.h>
#include <basicMPU6050.h>
#include <esp_system.h>
#include <BleMouse.h>
#include <Preferences.h>
#include <Wire.h>

basicMPU6050<> mpu;
BleMouse mouse("太・空・老・鼠");
Preferences mpu_prefs;

const int LMB_PIN = GPIO_NUM_2;
const int PW_MPU_PIN = GPIO_NUM_3;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN_AUX, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_BUILTIN_AUX, HIGH);

    pinMode(LMB_PIN, INPUT_PULLUP);

    pinMode(PW_MPU_PIN, OUTPUT);
    digitalWrite(PW_MPU_PIN, LOW);
    delay(60);

    mpu.setup();

    mpu_prefs.begin("SpaceMouse");
    if (mpu_prefs.isKey("mpu_calib")) {
        // printf("loading calibration...\n");
        mpu_prefs.getBytes("mpu_calib", &mpu, N_AXIS * sizeof(float));
    } else {
        // printf("calibrating...\n");
        mpu.setBias();
        mpu_prefs.putBytes("mpu_calib", &mpu, N_AXIS * sizeof(float));
    }
    mpu_prefs.end();

    mouse.begin();

    digitalWrite(LED_BUILTIN_AUX, LOW);
    digitalWrite(LED_BUILTIN, HIGH);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
}

uint32_t last_t = 0;
float x = 0;
float y = 0;
bool lmb_pressed = false;
uint32_t idle = 0;

const double PIXEL_PER_ANGLE = 1200 / radians(48.0);
const double DECAY_RATE = 0.92;
const uint32_t IDLE_MICROSECONDS = 5 * 60'000'000;

void loop()
{
    if (!mouse.isConnected()) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
        // printf("connecting...\n");
        last_t = 0;
        return;
    }
    digitalWrite(LED_BUILTIN, LOW);
    bool lmb = digitalRead(LMB_PIN) == LOW;

    mpu.updateBias();

    uint32_t t = micros();
    float dy = -mpu.gx();
    float dx = -mpu.gz();

    uint32_t dt = last_t ? t - last_t : 0;
    last_t = t;

    // printf("dt=%u x=%f y=%f dx=%f dy=%f lmb=%d/%d\n", dt, x, y, dx, dy, lmb, lmb_pressed);

    float scale = dt * (1e-6 * PIXEL_PER_ANGLE);
    x += dx * scale;
    y += dy * scale;
    int8_t rdx = round(constrain(x, INT8_MIN, INT8_MAX));
    int8_t rdy = round(constrain(y, INT8_MIN, INT8_MAX));
    if (rdx || rdy) {
        mouse.move(rdx, rdy);
        idle = 0;
    }
    if (lmb != lmb_pressed) {
        if (lmb) {
            mouse.press(MOUSE_LEFT);
        } else {
            mouse.release(MOUSE_LEFT);
        }
        idle = 0;
        mouse.setBatteryLevel(80);
    }
    x -= rdx;
    y -= rdy;
    x *= DECAY_RATE;
    y *= DECAY_RATE;
    lmb_pressed = lmb;

    idle += dt;
    if (idle > IDLE_MICROSECONDS) {
        digitalWrite(PW_MPU_PIN, HIGH);
        esp_deep_sleep_enable_gpio_wakeup(BIT(LMB_PIN),
                                          digitalRead(LMB_PIN) ?
                                          ESP_GPIO_WAKEUP_GPIO_LOW :
                                          ESP_GPIO_WAKEUP_GPIO_HIGH);
        esp_deep_sleep_start();
    }

    delay(15);
}
