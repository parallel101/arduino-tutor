#pragma once

#include <Arduino.h>

template <class T>
static void report_error(T const &value, const char *file, int line)
{
    printf("(%s:%d) Error: %s\n", file, line, String(value).c_str());
}

#define report_error(...) report_error(__VA_ARGS__, __FILE__, __LINE__)
