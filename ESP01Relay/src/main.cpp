#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "secrets.h"

static const char INDEX_HTML[] = R"(
<html>
  <head>
    <title>ESP32控制台</title>
  </head>
  <body>
    <h1>ESP32控制台</h1>
    <hr>
    <p>LED 灯操作：</p>
    <button id="on">开启</button>
    <button id="off">关闭</button>
  </body>
  <script>
    document.querySelector("#on").addEventListener("click", async () => {
      try {
        const response = await fetch("/on", {
          method: "POST",
        });
        console.log(`Response: ${response.status}`);
      } catch (e) {
        console.error(`Error: ${e}`);
      }
    });
    document.querySelector("#off").addEventListener("click", async () => {
      try {
        const response = await fetch("/off", {
          method: "POST",
        });
        console.log(`Response: ${response.status}`);
      } catch (e) {
        console.error(`Error: ${e}`);
      }
    });
  </script>
</html>
)";

AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    int status = WiFi.waitForConnectResult();
    if (status != WL_CONNECTED) {
        Serial.printf("WiFi connect failed, status=%d\n", status);
        return;
    }
    Serial.printf("WiFi connected, ip=%s, gateway=%s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str());

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);

    server.on("/", [] (AsyncWebServerRequest *request) {
        request->send(200, "text/html;charset=utf-8", INDEX_HTML);
    });
    server.on("/on", [] (AsyncWebServerRequest *request) {
        digitalWrite(0, HIGH);
        Serial.printf("turned on relay.\n");
        request->send(200, "text/plain", "OK");
    });
    server.on("/off", [] (AsyncWebServerRequest *request) {
        digitalWrite(0, LOW);
        Serial.printf("turned off relay.\n");
        request->send(200, "text/plain", "OK");
    });

    server.begin();
}

void loop() {
    delay(100);
}
