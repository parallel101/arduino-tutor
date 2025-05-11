#include <Arduino.h>

class SYN6288 {
    uint8_t busy;
    uint8_t bgm = 0;
    uint8_t encoding = 0;

public:
    explicit SYN6288(uint8_t busy = -1) : busy(busy) {
    }

    void setBGM(uint8_t bgm) {
        this->bgm = bgm;
    }

    void setEncoding(uint8_t encoding) {
        this->encoding = encoding;
    }

    void begin() {
        Serial.begin(9600);
    }

    bool available() {
        return digitalRead(busy);
    }

    void write(const char *str, uint16_t len) {
        uint8_t head[206];
        if (len > 200) {
            len = 200;
        }
        uint16_t bufLen = len + 3;

        head[0] = 0xfd;
        head[1] = bufLen >> 8;
        head[2] = bufLen & 0xff;
        head[3] = 0x01;
        head[4] = bgm << 3 | encoding;
        for (uint16_t i = 0; i < len; ++i) {
            head[5 + i] = str[i];
        }
        bufLen += 3;

        uint8_t checkSum = 0;
        for (uint16_t i = 0; i < bufLen - 1; ++i) {
            checkSum ^= head[i];
        }
        head[bufLen - 1] = checkSum;

        for (uint16_t i = 0; i < bufLen; ++i) {
            Serial.write(head[i]);
        }
    }

    void write(const char *str) {
        write(str, strlen(str));
    }

    void write(String str) {
        write(str.c_str(), str.length());
    }
};

SYN6288 syn;

void setup() {
    syn.begin();
}

void loop() {
    const char buf[] = "[v2]Hello, world";
    syn.write(buf, sizeof buf - 1);
    delay(4000);
}
