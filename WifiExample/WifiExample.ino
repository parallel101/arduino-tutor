#include <Arduino.h>
#include <WiFi.h>
#if 1
void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    WiFi.mode(WIFI_STA);
    WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
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
    delay(1000);
}
#else
void setup()
{
    Serial.begin(115200);

    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup done");
}

void loop()
{
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    delay(5000);
}
#endif
