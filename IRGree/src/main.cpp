#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Gree.h>

IRGreeAC ac(0);

// void onChange() {
//     static unsigned long t0;
//     unsigned long t = micros();
//     Serial.println(t - t0);
//     t0 = t;
// }

void printState() {
    Serial.println("GREE A/C remote is in the following state:");
    Serial.print("  ");
    Serial.println(ac.toString());
    uint8_t *ir_code = ac.getRaw();
    Serial.print("IR Code: 0x");
    for (uint8_t i = 0; i < kGreeStateLength; i++)
        Serial.print(ir_code[i], HEX);
    Serial.println();

    pinMode(1, INPUT);
    // attachInterrupt(digitalPinToInterrupt(2), onChange, CHANGE);
}

void setup() {
    ac.begin();
    Serial.begin(115200);
    delay(200);
    ac.calibrate();

    Serial.println("Default state of the remote.");
    printState();
    Serial.println("Setting desired state for A/C.");
    ac.on();
    ac.setFan(1);
    ac.setMode(kGreeCool);
    ac.setTemp(26);
    ac.setSwingVertical(false, kGreeSwingAuto);
    ac.setXFan(false);
    ac.setLight(false);
    ac.setSleep(false);
    ac.setTurbo(true);
}

void loop() {
    // unsigned long t0 = micros();
    // int ledValue = 0;
    // for (int i = 0; i < sizeof seq / sizeof seq[0]; ++i) {
    //     unsigned long t1 = t0 + seq[i];
    //     while (micros() < t1);
    //     digitalWrite(0, ledValue = !ledValue);
    //     t0 = t1;
    // }
    // digitalWrite(0, ledValue = 0);
    // delay(1000);

    delay(1000);
    Serial.println("Sending IR command to A/C ...");
    ac.send();
    printState();
}
