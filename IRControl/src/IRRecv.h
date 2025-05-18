#pragma once

#include <Arduino.h>

namespace IRRecv {

void attach(int irPin, bool activeHigh);
void setCallback(void (*callback)(uint8_t addr, uint8_t cmd, bool repeat));
bool getResult(uint8_t *addr, uint8_t *cmd);

}
