#include "IRRecv.h"
#include <Arduino.h>
#include <TimerOne.h>

namespace IRRecv {

#if USE_AVR_PORTS
static volatile uint8_t *irPinPort;
static uint8_t irPinMask;
#else
static int irPinIndex;
#endif
static bool irActiveHigh;

static uint8_t resultAddress;
static uint8_t resultCommand;
static volatile bool hasResult = false;

void (*callbackResult)(uint8_t addr, uint8_t cmd, bool repeat) = nullptr;

bool getResult(uint8_t *addr, uint8_t *cmd) {
    if (!hasResult) {
        return false;
    }
    hasResult = false;
    *addr = resultAddress;
    *cmd = resultCommand;
    return true;
}

static void onResult(uint32_t value) {
    // Serial.println(value, HEX);

    uint8_t address = (uint8_t)(value & 0xFF);
    uint8_t addressCheck = (uint8_t)((value >> 8) & 0xFF);
    uint8_t command = (uint8_t)((value >> 16) & 0xFF);
    uint8_t commandCheck = (uint8_t)((value >> 24) & 0xFF);
    if ((address ^ addressCheck) != 0xFF) {
        return;
    }
    if ((command ^ commandCheck) != 0xFF) {
        return;
    }
    resultAddress = address;
    resultCommand = command;
    hasResult = true;
    if (callbackResult) {
        callbackResult(address, command, false);
    }
    // Serial.print(address, HEX);
    // Serial.print(' ');
    // Serial.print(command, HEX);
    // Serial.println();
}

static void onRepeat() {
    hasResult = true;
    if (callbackResult) {
        callbackResult(resultAddress, resultCommand, true);
    }
    // Serial.println("REPEAT");
}

enum Phase {
    Idle,
    AGC,
    DataHigh,
    DataLow,
    RepeatHigh,
};

static volatile uint32_t usTick = 0;
static Phase phase = Idle;
static uint8_t bitCounter = 0;
static uint32_t bitValue = 0;

static void bitReset() {
    bitCounter = 0;
    bitValue = 0;
}

static void addBit(bool bit) {
    if (bit) {
        bitValue |= UINT32_C(1) << bitCounter;
    }
    ++bitCounter;
}

static bool bitOverflow() {
    if (bitCounter >= 32) {
        return true;
    }
    return false;
}

static uint32_t bitResult() {
    return bitValue;
}

#define check(_v, _us, _tol) voltage == (_v) && dt >= (_us) - (_tol) && dt <= (_us) + (_tol)

static void ISRHandlerIR() {
    uint32_t dt = usTick;
    usTick = 0;

#if USE_AVR_PORTS
    bool voltage = *irPinPort & irPinMask;
#else
    bool voltage = digitalRead(irPinIndex);
#endif
    voltage = voltage == irActiveHigh;

    // Serial.print(voltage);
    // Serial.print(' ');
    // Serial.println(dt);

    if (check(HIGH, 9000, 560)) {
        phase = AGC;
        // Serial.println("AGC");
        return;
    }

    switch (phase) {
        default:
            phase = Idle;
            // Serial.println("Idle");
            return;

        case AGC:
            if (check(LOW, 4500, 500)) {
                bitReset();
                phase = DataHigh;
                // Serial.println("DataHigh");
                return;
            }
            if (check(LOW, 2250, 400)) {
                phase = RepeatHigh;
                // Serial.println("RepeatHigh");
                return;
            }
            break;

        case RepeatHigh:
            if (check(HIGH, 560, 350)) {
                onRepeat();
                phase = Idle;
                // Serial.println("Idle");
                return;
            }
            break;

        case DataHigh:
            if (check(HIGH, 560, 350)) {
                if (bitOverflow()) {
                    onResult(bitResult());
                    phase = Idle;
                } else {
                    phase = DataLow;
                }
                return;
            }
            break;

        case DataLow:
            if (check(LOW, 2250 - 560, 350)) {
                addBit(1);
                phase = DataHigh;
                // Serial.println("DataHigh 1");
                return;
            }
            if (check(LOW, 1120 - 560, 350)) {
                addBit(0);
                phase = DataHigh;
                // Serial.println("DataHigh 0");
                return;
            }
            break;
    }
    // Serial.println("??");
}

#undef check

static void timer_ISR() {
    usTick += 10;
}

void setCallback(void (*callback)(uint8_t addr, uint8_t cmd, bool repeat)) {
    callbackResult = callback;
}

void attach(int irPin, bool activeHigh) {
    Serial.begin(115200);

    Timer1.attachInterrupt(timer_ISR);
    Timer1.initialize(10);

#if USE_AVR_PORTS
    irPinPort = portOutputRegister(digitalPinToPort(irPin));
    irPinMask = digitalPinToBitMask(irPin);
#else
    irPinIndex = irPin;
#endif
    irActiveHigh = activeHigh;

    pinMode(irPin, activeHigh ? INPUT : INPUT_PULLUP);
    int intr = digitalPinToInterrupt(irPin);
    if (intr != NOT_AN_INTERRUPT) {
        attachInterrupt(intr, ISRHandlerIR, CHANGE);
    }
}

}
