#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

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
  server.on("/", [] {
    Serial.println("received / request!");
    server.send(200, "text/html;charset=utf-8", INDEX_HTML);
  });

WebServer server(80);

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connecting...");
        delay(200);
    }
    Serial.println("WiFi connected.");

    server.on("/set_state", [] {
        String enable = server.arg("enable");
        if (enable == "on") {
            digitalWrite(0, HIGH);
        } else if (enable == "off") {
            digitalWrite(0, LOW);
        }
    });
    server.on("/set_state", [] {
        String enable = server.arg("enable");
        if (enable == "on") {
            digitalWrite(0, HIGH);
        } else if (enable == "off") {
            digitalWrite(0, LOW);
        }
    });
}

void loop() {
    server.handleClient();
}
