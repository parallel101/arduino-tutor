#include <DHT11.h>

DHT11 dht(11);

void setup() {
  Serial.begin(9600);
}

void loop() {
  int h = dht.readHumidity();
  if (h == DHT11::ERROR_CHECKSUM || h == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(h));
    return;
  }
  int t = dht.readTemperature();
  if (t == DHT11::ERROR_CHECKSUM || t == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(t));
    return;
  }
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temperature: ");
  Serial.println(t);
}
