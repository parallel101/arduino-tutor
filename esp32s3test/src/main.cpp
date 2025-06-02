#include <Arduino.h>

void setup() {
    pinMode(RGB_BUILTIN, OUTPUT);

    psramInit();
}

void loop() {
    printf("Hello, ESP32!\n");
    delay(500);
    neopixelWrite(RGB_BUILTIN, 20, 10, 0);
    delay(500);
    neopixelWrite(RGB_BUILTIN, 10, 20, 0);

    if (!ps_malloc(32 * 1024)) {
        neopixelWrite(RGB_BUILTIN, 30, 0, 30);
        delay(1000);
    }
}
