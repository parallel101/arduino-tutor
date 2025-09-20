#include <Arduino.h>
#include <BleMouse.h>

const int VRx = 1;
const int VRy = 0;
const int SW = 6;


BleMouse mouse("ESPMouse");


void setup() {
    // put your setup code here, to run once:
    pinMode(VRx, ANALOG);
    pinMode(VRy, ANALOG);
    pinMode(SW, INPUT);
    Serial.begin(115200);
    analogSetAttenuation(ADC_11db);
    mouse.begin();
}

// 1665

void loop() {
    // put your main code here, to run repeatedly:
    int32_t vrx = analogReadMilliVolts(VRx);
    int32_t vry = analogReadMilliVolts(VRy);
    int sw = digitalRead(SW);

    vrx -= 1665;
    vry -= 1665;

    if (abs(vrx) < 100) {
        vrx = 0;
    }
    if (abs(vry) < 100) {
        vry = 0;
    }

    vrx = -constrain(vrx, -1000, 1000);
    vry = -constrain(vry, -1000, 1000);

    if (vrx != 0 || vry != 0) {
        double x = tanh(vrx / 1000.0) * 50.0;
        double y = tanh(vry / 1000.0) * 50.0;

        Serial.printf("%f %f\n", x, y);
        mouse.move(round(x), round(y));
    }

    if (sw == LOW) {
        Serial.printf("press\n");
        mouse.press(MOUSE_LEFT);
    } else {
        mouse.release(MOUSE_LEFT);
    }

    delay(30);
}
