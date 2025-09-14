#include <Arduino.h>
// #include <BleMouse.h>
#include <esp_sleep.h>
#include "BatteryPercent.h"

// BleMouse bleMouse("ESPMouse", "Espressif", 100);

const int CLK = 2;
const int DT = 3;
const int PWR = A0;

bool connected = false;
int idle = 0;
volatile bool changed = false;


void IRAM_ATTR onBatteryUpdate()
{
    int percent = batteryVoltageToPercentage(analogReadMilliVolts(PWR) * 10);
    if (connected) {
        // bleMouse.setBatteryLevel(percent);
    }
}

void IRAM_ATTR onClkRise()
{
    uint32_t pins = GPIO.in.data;
    bool clk = pins & BIT(CLK);
    bool dt = pins & BIT(DT);
    if (!clk) {
        return;
    }
    int direction;
    if (dt == LOW) {
        direction = 1;
    } else {
        direction = -1;
    }
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    // if (connected) {
        // bleMouse.move(0, 0, direction, 0);
    // }

    changed = true;
}

void setup() {
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    attachInterrupt(CLK, onClkRise, RISING);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // bleMouse.begin();

    hw_timer_t *batteryTimer = timerBegin(1, 80, true);
    timerAttachInterrupt(batteryTimer, onBatteryUpdate, false);
    timerAlarmWrite(batteryTimer, 5'000'000, true);
    timerAlarmEnable(batteryTimer);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    setCpuFrequencyMhz(80);
}

void loop() {
    // if (!connected) {
    //     if (!bleMouse.isConnected()) {
    //         digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    //         Serial.println("connecting...");
    //         delay(500);
    //         return;
    //     } else {
    //         connected = true;
    //         digitalWrite(LED_BUILTIN, LOW);
    //     }
    // }

    delay(1000);

    if (changed) {
        changed = false;
        idle = 0;
    } else {
        ++idle;
    }

    if (idle > 5) {
        esp_deep_sleep_enable_gpio_wakeup(BIT(CLK),
                                          digitalRead(CLK) ?
                                          ESP_GPIO_WAKEUP_GPIO_LOW :
                                          ESP_GPIO_WAKEUP_GPIO_HIGH);
        esp_deep_sleep_start();
    }
}
