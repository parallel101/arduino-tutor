#include <WiFi.h>
#include <WebServer.h>
#include <MySecrets.h>

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

WebServer server(80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connecting...");
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);  // off
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);   // on
  }
  Serial.println("WiFi connected.");
  digitalWrite(LED_BUILTIN, HIGH);   // off

  Serial.println(WiFi.localIP());

  server.on("/", [] {
    Serial.println("received / request!");
    server.send(200, "text/html;charset=utf-8", INDEX_HTML);
  });
  server.on("/on", [] {
    Serial.println("received /on request!");
    digitalWrite(LED_BUILTIN, LOW);   // on
    server.send(200, "text/html;charset=utf-8", "OK");
  });
  server.on("/off", [] {
    Serial.println("received /off request!");
    digitalWrite(LED_BUILTIN, HIGH);   // off
    server.send(200, "text/html;charset=utf-8", "OK");
  });
  server.begin();
}

void loop() {
  server.handleClient();
}
