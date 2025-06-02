#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

void netSetup()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    bool led = true;
    while (true) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            printf("WiFi connected\n");
            break;
        } else {
            printf("WiFi connecting, status=%d\n", status);
            led = !led;
            delay(100);
        }
    }
}

// void netLoop()
// {
//     while (WiFi.status() != WL_CONNECTED) {
//         bool led = false;
//         digitalWrite(LED_BUILTIN, LOW);
//         printf("WiFi disconnected, reconnecting...\n");
//         WiFi.reconnect();
//         while (WiFi.status() != WL_CONNECTED) {
//             digitalWrite(LED_BUILTIN, led = !led);
//             delay(200);
//         }
//         digitalWrite(LED_BUILTIN, HIGH);
//     }
//     delay(1000);
// }
