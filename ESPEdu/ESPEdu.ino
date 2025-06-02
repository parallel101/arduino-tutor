#include <WiFi.h>
#include <HTTPClient.h>
#include <MySecrets.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connecting...");
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);  // off
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);   // on
  }
  Serial.println("WiFi connected.");
  digitalWrite(LED_BUILTIN, HIGH);   // off
}

void loop() {
  delay(1000);
  HTTPClient http;
  http.begin("http://142857.red:8080/index.html");
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    Serial.println(body);
  } else {
    Serial.printf("HTTP error: %d\n", code);
  }
}
