#include "Thermometer.h"
#include <DHT.h>

static DHT dht;
static bool isDHTReady = false;

void thermoSetup() {
    dht.setup(1);
}

void thermoRead(float &humidity, float &temperature) {
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
}
