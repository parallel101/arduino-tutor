#include <Arduino.h>
#include <HTTPClient.h>
#include <Json.h>
#include "secrets.h"

static String get_access_token()
{
    static const char url[] = "http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=" BAIDU_API_KEY "&client_secret=" BAIDU_SECRET_KEY;

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    int code;
    do {
        code = http.POST("");
    } while (code < 0);
    if (code != 200) {
        http.end();
        Serial.println(code);
        return "";
    }
    String body = http.getString();
    http.end();
    Json json(body);
    int errNo = json.getElement("err_no").toInt();
    if (errNo != 0) {
        Serial.println(json.getElement("err_msg"));
        return "";
    }
    return json.getElement("access_token");
}

static String speech_reco(String const &token, uint8_t *audio, size_t size)
{
    String url = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=" BAIDU_CUID "&token=" + token;
;

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "audio/pcm;rate=16000");
    int code;
    do {
        code = http.POST(audio, size);
    } while (code < 0);
    if (code != 200) {
        http.end();
        Serial.println(code);
        return "";
    }
    String body = http.getString();
    http.end();
    Json json(body);
    int errNo = json.getElement("err_no").toInt();
    if (errNo != 0) {
        Serial.println(json.getElement("err_msg"));
        return "";
    }
    return json.getElement("result").toArray().getElement(0).toString();
}

static String token;

void cloudSetup()
{
    do {
        token = get_access_token();
    } while (token.isEmpty());
}

String cloudQuery(uint8_t *audio, size_t size)
{
    return speech_reco(token, audio, size);
}
