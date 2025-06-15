#pragma once

#include <Arduino.h>

struct Memo
{
    String content;
    String time;
};

void memoSetup();
