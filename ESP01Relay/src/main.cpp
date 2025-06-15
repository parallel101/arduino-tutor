#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>

static const char INDEX_HTML[] = R"(
<html>
  <head>
    <title>ESP-01S 控制台</title>
  </head>
  <body>
    <h1>ESP-01S 控制台</h1>
    <hr>
    <p>设置插座状态：</p>
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

ESP8266WebServer server(80);
AutoConnect portal(server);

void setup() {
    Serial.begin(115200);

    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);

    server.on("/", [] () {
        server.send(200, "text/html;charset=utf-8", INDEX_HTML);
    });
    server.on("/on", [] () {
        digitalWrite(0, LOW);
        Serial.printf("turned on relay.\n");
        server.send(200, "text/plain", "OK");
    });
    server.on("/off", [] () {
        digitalWrite(0, HIGH);
        Serial.printf("turned off relay.\n");
        server.send(200, "text/plain", "OK");
    });
    server.on("/esp", [] () {
        JsonDocument info;
        info["mac_addr"] = WiFi.macAddress();
        info["build"] = __DATE__ " " __TIME__;
        info["product"] = "relay";
        info["name"] = "继电器";
        info["descrption"] = "智能插座/继电器模块，控制单个设备的开关";
        info["chip"] = "esp01_1m";
        String infoStr;
        serializeJson(info, infoStr);
        server.send(200, "text/plain", infoStr);
    });

    if (portal.begin()) {
        Serial.printf("WiFi connected, ip=%s, gateway=%s\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.gatewayIP().toString().c_str());
    }
}

void loop() {
    portal.handleClient();
}
