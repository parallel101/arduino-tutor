#include <Arduino.h>
#include <TimerOne.h>

const int IR_PIN = 2;

void IR_Result(int32_t value) {
    Serial.print("IR Result: ");
    Serial.println(value);
}

void IR_Repeat(int32_t value) {
    Serial.println("IR Repeat");
}

static uint32_t t0 = 0;
static uint32_t value = 0;
static uint32_t shift = 1;
static enum Phase {
    Idle,
    AGC,
    LP,
    Brust,
    Repeat,
} phase = Idle;

bool volt0 = LOW;

void IR_ISR() {
    uint32_t t1 = micros();
    uint32_t dt = t1 - t0;

    bool voltage = digitalRead(IR_PIN) == LOW;
    if (volt0 == voltage) {
        return;
    }
    volt0 = voltage;

    if (dt < 0) {
        Serial.print(t0);
        Serial.print(' ');
        Serial.print(t1);
        Serial.print(' ');
        Serial.println(dt);
    }
    t0 = t1;

    if (voltage == LOW && 8000 <= dt && dt <= 10000) {
        phase = AGC;
        return;
    }

    if (phase == AGC) {
        if (voltage == HIGH && 1700 <= dt && dt <= 2800) {
            phase = Repeat;
            return;
        }

        if (voltage == HIGH && 3500 <= dt && dt <= 5500) {
            phase = LP;
            value = 0;
            shift = 1;
            return;
        }
    }

    if (phase == Repeat) {
        if (voltage == LOW && 400 <= dt && dt <= 750) {
            IR_Repeat(value);
        }
    }

    if (phase == LP) {
        if (voltage == LOW && 400 <= dt && dt <= 750) {
            if (shift != 0) {
                phase = Brust;
            } else {
                IR_Result(value);
                phase = Idle;
            }
            return;
        }
    }

    if (phase == Brust) {
        if (voltage == HIGH && 400 <= dt && dt <= 750) {
            phase = LP;
            shift <<= 1;
            return;
        }
        if (voltage == HIGH && 1500 <= dt && dt <= 1700) {
            phase = LP;
            value += shift;
            shift <<= 1;
            return;
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(IR_PIN, INPUT);
    int intr = digitalPinToInterrupt(IR_PIN);
    if (intr != NOT_AN_INTERRUPT) {
        attachInterrupt(intr, IR_ISR, CHANGE);
    }
}

void loop() {
    delay(1000);
}
