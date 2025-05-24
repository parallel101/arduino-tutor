#include "CloudSTT.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <Json.h>
#include "BufferStream.h"
#include "secrets.h"

template <class T>
static void report_error(T const &value, const char *file, int line)
{
    Serial.printf("(%s:%d) Error: ", file, line);
    Serial.println(value);
}

#define report_error(...) report_error(__VA_ARGS__, __FILE__, __LINE__)

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
    do {
        code = http.POST(audio, size);
    } while (code < 0);
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
    http.collectHeaders(collect_headers, sizeof(collect_headers) / sizeof(collect_headers[0]));
    int code;
    do {
        code = http.GET();
    } while (code < 0);
    if (code != 200) {
        http.end();
        report_error(code);
        return 0;
    }
    String content_type = http.header("Content-Type");
    if (content_type == "audio/basic;codec=pcm;rate=16000;channel=1") {
        BufferStream stream(buffer, size);
        // stream.reserve(http.getSize());
        http.writeToStream(&stream);
        http.end();
        if (stream.isEmpty()) {
            report_error("body is empty");
            return {};
        }
        return stream.size();

    // } else if (content_type == "audio/wav") {
    //     StreamString stream;
    //     stream.reserve(http.getSize());
    //     http.writeToStream(&stream);
    //     http.end();
    //     if (stream.available() <= 0x2a) {
    //         report_error("body too short");
    //         return {};
    //     }
    //     return stream.substring(0x2a);

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
    do {
        token = get_access_token();
    } while (token.isEmpty());
}

String cloudQuery(uint8_t *audio, size_t size)
{
    return speech_reco(token, audio, size);
}

size_t cloudSynth(uint8_t *buffer, size_t size, String const &text, String const &options)
{
    return speech_synth(token, buffer, size, text, options);
}
