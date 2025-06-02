#include <Arduino.h>
#include <esp_system.h>

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(GPIO_NUM_0, INPUT_PULLDOWN);

    gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_ON);
}

void loop() {
    digitalWrite(LED_BUILTIN, LOW);

    while (digitalRead(GPIO_NUM_0) != LOW) {
        delay(500);
    }

    Serial.println("Sleepy now");
    Serial.flush();
    digitalWrite(LED_BUILTIN, HIGH);
    esp_light_sleep_start();
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Wake up from sleep!");
    Serial.printf("Wakeup reason: %d\n", esp_sleep_get_wakeup_cause());
}
