#include "CloudSTT.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <Json.h>
#include <cstring>
#include "BufferStream.h"
#include "secrets.h"
#include "report_error.h"

#define MAX_RETRIES 5
#define RETRY_DELAY 100

static String get_access_token()
{
    static const char url[] = "http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=" BAIDU_VOP_API_KEY "&client_secret=" BAIDU_VOP_SECRET_KEY;

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    int code;
    for (int tries = 0; tries != MAX_RETRIES; ++tries) {
        code = http.POST({});
        if (code >= 0) {
            break;
        }
        delay(RETRY_DELAY);
    }
    if (code != 200) {
        http.end();
        report_error(code);
        return "";
    }
    String body = http.getString();
    http.end();
    Json json(body);
    int errNo = json.getElement("err_no").toInt();
    if (errNo != 0) {
        report_error(json.getElement("err_msg"));
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
    for (int tries = 0; tries != MAX_RETRIES; ++tries) {
        code = http.POST(audio, size);
        if (code >= 0) {
            break;
        }
        delay(RETRY_DELAY);
    }
    if (code != 200) {
        http.end();
        report_error(code);
        return "";
    }
    String body = http.getString();
    http.end();
    Json json(body);
    int errNo = json.getElement("err_no").toInt();
    if (errNo != 0) {
        report_error(json.getElement("err_msg"));
        return "";
    }
    return json.getElement("result").toArray().getElement(0).toString();
}

static bool is_char_url_safe(char c) {
    if ('0' <= c && c <= '9') {
        return true;
    }
    if ('a' <= c && c <= 'z') {
        return true;
    }
    if ('A' <= c && c <= 'Z') {
        return true;
    }
    if (c == '-' || c == '_' || c == '.') {
        return true;
    }
    return false;
}

static String url_encode(String const &text)
{
    static const char lut[] = "0123456789ABCDEF";
    String res;
    for (char c: text) {
        if (is_char_url_safe(c)) {
            res += c;
        } else {
            res += '%';
            res += lut[(uint8_t)c >> 4];
            res += lut[(uint8_t)c & 0xF];
        }
    }
    return res;
}

static size_t speech_synth(String const &token, uint8_t *buffer, size_t size, String const &text, String const &options)
{
    String url = "http://tsn.baidu.com/text2audio?tex=" + url_encode(text) + options + "&aue=4&lan=zh&ctp=1&audio_ctrl={\"sampling_rate\":16000}&cuid=" BAIDU_CUID "&tok=" + token;

    HTTPClient http;
    http.begin(url);
    static const char *collect_headers[] = {"Content-Type"};
    http.collectHeaders(collect_headers, sizeof collect_headers / sizeof collect_headers[0]);
    int code;
    for (int tries = 0; tries != MAX_RETRIES; ++tries) {
        code = http.GET();
        if (code >= 0) {
            break;
        }
        delay(RETRY_DELAY);
    }
    if (code != 200) {
        http.end();
        report_error(code);
        return 0;
    }
    String content_type = http.header("Content-Type");
    if (content_type == "audio/basic;codec=pcm;rate=16000;channel=1") {
        BufferStream stream(buffer, size);
        stream.reserve(http.getSize());
        http.writeToStream(&stream);
        http.end();
        if (stream.isEmpty()) {
            report_error("body is empty");
            return {};
        }
        return stream.size();

    } else if (content_type == "audio/wav") {
        BufferStream stream(buffer, size);
        stream.reserve(http.getSize());
        http.writeToStream(&stream);
        http.end();
        if (stream.isEmpty()) {
            report_error("body is empty");
            return {};
        }
        memset(buffer, 0, 0x2a);
        return stream.size();

    } else if (content_type == "application/json") {
        String body = http.getString();
        http.end();
        Json json(body);
        report_error(json.getElement("err_msg"));
        return 0;

    } else {
        report_error(content_type);
        http.end();
        return 0;
    }
}

static String token;

void cloudSetup()
{
    for (int tries = 0; tries != MAX_RETRIES; ++tries) {
        token = get_access_token();
        if (!token.isEmpty()) {
            break;
        }
        delay(RETRY_DELAY);
    }
    if (token.isEmpty()) {
        report_error("failed to get access token");
    }
}

String cloudQuery(uint8_t *audio, size_t size)
{
    return speech_reco(token, audio, size);
}

size_t cloudSynth(uint8_t *buffer, size_t size, String const &text, String const &options)
{
    return speech_synth(token, buffer, size, text, options);
}
