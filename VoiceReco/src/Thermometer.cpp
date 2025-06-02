#include "Thermometer.h"
#include "AIChat.h"
#include <DHT.h>

#define DHT_PIN GPIO_NUM_13

static DHT *dht;
static bool isDHTReady = false;

static String get_temperature(JsonDocument const &arguments) {
    float humidity = 0;
    float temperature = 0;
    thermoRead(humidity, temperature);

    JsonDocument result;
    result["humidity"] = humidity;
    result["temperature"] = temperature;

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

void thermoSetup() {
    dht = new DHT;
    dht->setup(DHT_PIN);
    registerTool(Tool{
        .name = "get_temperature",
        .descrption = "获取房间温度和湿度",
        .parameters = {
        },
        .callback = get_temperature,
    });
}

void thermoRead(float &humidity, float &temperature) {
    humidity = dht->getHumidity();
    temperature = dht->getTemperature();
}
