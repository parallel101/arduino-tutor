#include <Arduino.h>
#include "BatteryPercent.h"
// #include <BleMouse.h>
// #include <esp_sleep.h>

// BleMouse bleMouse("ESPMouse", "Espressif", 100);

const int CLK = 7;
const int DT = 6;
const int PWR = A0;

bool connected = false;
volatile bool changed = false;
int idle = 0;


void onClkRise()
{
    // uint32_t pins = GPIO.in.data;
    // bool clk = pins & (1 << CLK);
    // bool dt = pins & (1 << DT);
    // if (!clk) {
    //     return;
    // }
    // int direction;
    // if (dt == LOW) {
    //     direction = 1;
    // } else {
    //     direction = -1;
    // }
    // digitalPinToAnalogChannel(PWR);
    //
    // if (connected) {
    //     bleMouse.move(0, 0, direction, 0);
    // }
}

        

void setup() {
    // pinMode(CLK, INPUT);
    // pinMode(DT, INPUT);
    // attachInterrupt(CLK, onClkRise, RISING);
    //
    // pinMode(LED_BUILTIN, OUTPUT);
    // digitalWrite(LED_BUILTIN, LOW);
    //
    // bleMouse.begin();
    //
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    // setCpuFrequencyMhz(24);

    Serial.begin();
}

void loop() {
    delay(500);
    int percent = batteryVoltageToPercentage(analogReadMilliVolts(PWR) * 10);
    Serial.println(percent);

    // if (!connected) {
    //     if (!bleMouse.isConnected()) {
    //         digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    //         Serial.println("connecting...");
    //         return;
    //     } else {
    //         connected = true;
    //         digitalWrite(LED_BUILTIN, HIGH);
    //     }
    // }
    //
    // int latency = 1'000;
    // if (idle > 6'000) {
    //     esp_deep_sleep_enable_gpio_wakeup(1 << CLK,
    //                                       digitalRead(CLK) ?
    //                                       ESP_GPIO_WAKEUP_GPIO_LOW :
    //                                       ESP_GPIO_WAKEUP_GPIO_HIGH);
    //     esp_deep_sleep_start();
    // }
    //
    // delay(latency);
    //
    // bool chg = changed;
    // changed = 0;
    // if (chg) {
    //     idle = 0;
    // } else {
    //     idle += latency;
    // }
}
