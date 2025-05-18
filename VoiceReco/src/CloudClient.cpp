#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
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

void loop() {
    while (WiFi.status() != WL_CONNECTED) {
        bool led = false;
        digitalWrite(LED_BUILTIN, LOW);
        WiFi.reconnect();
        Serial.println("WiFi disconnected, reconnecting...");
        while (WiFi.status() != WL_CONNECTED) {
            digitalWrite(LED_BUILTIN, led = !led);
            delay(200);
        }
        digitalWrite(LED_BUILTIN, HIGH);
    }

    String url = "http://web.simmons.edu/";
    // }

    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();

    delay(1000);
}
