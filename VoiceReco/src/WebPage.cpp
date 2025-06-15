#include "WebPage.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <AutoConnect.h>
#include "secrets.h"

static const char INDEX_HTML[] = R"(
<html>
  <head>
    <title>ESP32-S3 控制台</title>
  </head>
  <body>
    <h1>ESP32-S3 控制台</h1>
    <hr>
    <p>指令：</p>
    <button class="exec-button">打开空调</button>
    <button class="exec-button">空调提高</button>
    <button class="exec-button">空调降低</button>
  </body>
  <script>
    document.querySelector(".exec-button").addEventListener("click", async () => {
      try {
        const response = await fetch("/exec", {
          method: "POST",
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
          },
          body: `cmd${encodeURIComponent(this.innerText)}`,
        });
        console.log(`Response: ${response.status}`);
      } catch (e) {
        console.error(`Error: ${e}`);
      }
    });
  </script>
</html>
)";

static WebServer server(80);
static AutoConnect portal(server);
static TaskHandle_t webTask;

static void web_server(void *)
{
    delay(100);
    while (true) {
        portal.handleClient();
    }
}

void webDiscoverDevices()
{
    // todo
}

void webSetup()
{
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    server.on("/", [] () {
        server.send(200, "text/html;charset=utf-8", INDEX_HTML);
    });
    server.on("/esp", [] () {
        JsonDocument info;
        info["mac_addr"] = WiFi.macAddress();
        info["build"] = __DATE__ " " __TIME__;
        info["product"] = "assistant";
        info["name"] = NICK_NAME;
        info["descrption"] = "语音助手，接受用户语音输入，发送指令到从设备";
        info["chip"] = "esp32-s3-devkitc-1-n16r8v";
        String infoStr;
        serializeJson(info, infoStr);
        server.send(200, "text/plain", infoStr);
    });
    // server.on("/exec", [] () {
    //     auto cmd = server.arg("cmd");
    //     printf("Executing command: [%s]\n", cmd.c_str());
    //     auto res = aiChat(cmd);
    //     printf("Command response: [%s]\n", res.c_str());
    //     server.send(200, "text/plain", res);
    // });

    if (portal.begin()) {
        printf("WiFi connected, ip=%s, gateway=%s, mask=%s\n",
               WiFi.localIP().toString().c_str(),
               WiFi.gatewayIP().toString().c_str(),
               WiFi.subnetMask().toString().c_str());
    }

    xTaskCreate(web_server, "web_server", 10 * 1024, nullptr, 1, &webTask);
}
