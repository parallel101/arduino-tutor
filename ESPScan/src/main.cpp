#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <secrets.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_11dBm);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connecting...");
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
    }
    Serial.println("WiFi connected.");
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print("localIP: ");
    Serial.println(WiFi.localIP());
    Serial.print("gatewayIP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("broadcastIP: ");
    Serial.println(WiFi.broadcastIP());
    Serial.print("subnetMask: ");
    Serial.println(WiFi.subnetMask());
}

void loop() {
    delay(1000);

    uint32_t mask = htonl(WiFi.subnetMask());
    uint32_t lan0 = (htonl(WiFi.broadcastIP()) & mask) + 1;
    uint32_t lanNum = ~mask - 1;
    if (lanNum <= 0) {
        return;
    }
    if (lanNum > 1024) {
        lanNum = 1024;
    }

    enum State : uint8_t {
        Idle,
        Connecting,
        Connected,
        Timeout,
        Error,
    };

    Serial.print("testing range: ");
    Serial.print(IPAddress(ntohl(lan0)).toString());
    Serial.print('~');
    Serial.println(IPAddress(ntohl(lan0 + lanNum)).toString());

    static SemaphoreHandle_t mtx = xSemaphoreCreateMutex();

    State *hosts = new State[lanNum];
    for (uint32_t i = 0; i != lanNum; ++i) {
        hosts[i] = Idle;
    }

    uint32_t progress = 0;
    const uint32_t CHUNK_SIZE = CONFIG_LWIP_MAX_ACTIVE_TCP - 1;
    while (true) {
        uint32_t chunkSize = min(progress + CHUNK_SIZE, lanNum) - progress;
        if (chunkSize == 0) {
            break;
        }

        delay(100);
        for (uint32_t i = progress; i < progress + chunkSize; ++i) {
            IPAddress ip = ntohl(lan0 + i);
            Serial.print("testing: ");
            Serial.println(ip.toString());

            hosts[i] = Connecting;
            AsyncClient *client = new AsyncClient;
            client->setAckTimeout(500);
            client->setRxTimeout(2);
            client->onConnect([] (void *arg, AsyncClient *client) {
                xSemaphoreTake(mtx, portMAX_DELAY);
                *(State *)arg = Connected;
                xSemaphoreGive(mtx);
                client->close();
                delete client;
            }, hosts + i);
            client->onError([] (void *arg, AsyncClient *client, int8_t error) {
                xSemaphoreTake(mtx, portMAX_DELAY);
                *(State *)arg = Error;
                xSemaphoreGive(mtx);
                delete client;
            }, hosts + i);
            client->onTimeout([] (void *arg, AsyncClient *client, int8_t time) {
                xSemaphoreTake(mtx, portMAX_DELAY);
                *(State *)arg = Timeout;
                xSemaphoreGive(mtx);
                client->close();
                delete client;
            }, hosts + i);

            client->connect(ip, 80);
        }

        while (true) {
            int connects = 0;
            int errors = 0;
            Serial.print('[');
            xSemaphoreTake(mtx, portMAX_DELAY);
            for (uint32_t i = progress; i < progress + chunkSize; ++i) {
                char c = '?';
                switch (hosts[i]) {
                    case Connected:
                        c = '*';
                        ++connects;
                        break;
                    case Error:
                        ++errors;
                        c = 'e';
                        break;
                    case Timeout:
                        ++errors;
                        c = '-';
                        break;
                    case Connecting:
                        c = '.';
                        break;
                    case Idle:
                        c = ' ';
                        break;
                }
                Serial.print(c);
            }
            xSemaphoreGive(mtx);
            Serial.print(']');
            Serial.println();

            if (connects + errors == chunkSize) {
                break;
            }
            delay(1000);
        }

        progress += chunkSize;
    }

    Serial.println();
    Serial.println("found devices:");

    HTTPClient http;
    for (uint32_t i = 0; i != lanNum; ++i) {
        if (hosts[i] == Connected) {
            IPAddress ip = ntohl(lan0 + i);
            Serial.print("  ");
            Serial.print(ip);
            Serial.print(": ");

            http.begin(ip.toString(), 80, "/esp");
            int code = http.GET();
            if (code != 200) {
                Serial.println("ERROR");
                continue;
            }
            Serial.println(http.getString());
        }
    }
    delete[] hosts;
}
