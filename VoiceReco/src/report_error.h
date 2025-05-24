#pragma once

#include <Arduino.h>

template <class T>
static void report_error(T const &value, const char *file, int line)
{
    Serial.printf("(%s:%d) Error: ", file, line);
    Serial.println(value);
}

#define report_error(...) report_error(__VA_ARGS__, __FILE__, __LINE__)
