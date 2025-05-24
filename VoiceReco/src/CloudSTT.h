#pragma once

#include <Arduino.h>

void cloudSetup();
String cloudQuery(uint8_t *audio, size_t size);
size_t cloudSynth(uint8_t *buffer, size_t size, void (*callback)(size_t bytes_read), String const &text, String const &options);
