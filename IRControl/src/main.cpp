#include <Arduino.h>
#include "IRRecv.h"

const int IR_PIN = 2;

void irCallback(uint8_t addr, uint8_t cmd, bool repeat) {
    Serial.print(addr, HEX);
    Serial.print(' ');
    Serial.print(cmd, HEX);
    Serial.print(' ');
    Serial.print(repeat);
    Serial.println();
}

void setup() {
    Serial.begin(115200);

    IRRecv::attach(IR_PIN, HIGH);
    IRRecv::setCallback(irCallback);
}

void loop() {
    delay(1000);
}
