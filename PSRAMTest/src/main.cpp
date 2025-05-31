#ifndef __XTENSA__
#error XTENSA EXPECTED
#endif
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    psramInit();
}

void loop() {
    Serial.printf("PSRAM Found: %d\n", psramFound());
    Serial.printf("PSRAM: %d/%d\n", ESP.getFreePsram(), ESP.getPsramSize());
    Serial.printf("Heap: %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf("Heap: %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    delay(1000);
    ps_malloc(100);
}
