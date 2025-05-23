#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

void netSetup()
{
    digitalWrite(LED_BUILTIN, LOW);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_7dBm);
    bool led = false;
    while (true) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            Serial.println("WiFi connected");
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        } else {
            Serial.print("WiFi connecting, status=");
            Serial.println(status);
            digitalWrite(LED_BUILTIN, (led = !led));
            delay(200);
        }
    }
}

void netLoop()
{
    while (WiFi.status() != WL_CONNECTED) {
        bool led = false;
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("WiFi disconnected, reconnecting...");
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) {
            digitalWrite(LED_BUILTIN, led = !led);
            delay(200);
        }
        digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(1000);
}
