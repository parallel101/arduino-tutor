#include "CloudSTT.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
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
    JsonDocument doc;
    deserializeJson(doc, body);
    int errNo = doc["err_no"];
    if (errNo != 0) {
        report_error(doc["err_msg"].as<String>());
        return "";
    }
    return doc["access_token"];
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
    JsonDocument doc;
    deserializeJson(doc, body);
    int errNo = doc["err_no"];
    if (errNo != 0) {
        report_error(doc["err_msg"].as<String>());
        return "";
    }
    return doc["result"][0];
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

static size_t speech_synth(String const &token, uint8_t *buffer, size_t size, void (*callback)(size_t bytes_read), String const &text, String const &options)
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
        WiFiClient &stream = http.getStream();
        size_t content_length = http.getSize();
        uint8_t *buf_write_ptr = buffer;
        uint8_t *buf_end_ptr = buffer + size;
        if (content_length == 0) {
            report_error("body is empty");
            return {};
        }
        if (content_length == -1) {
            report_error("cannot determine body size");
        }
        while (content_length) {
            while (buf_write_ptr != buf_end_ptr && content_length) {
                size_t want_read = std::min<size_t>(buf_end_ptr - buf_write_ptr, content_length);
                // size_t bytes_read = stream.readBytes(buf_write_ptr, want_read);
                int bytes_read = stream.read(buf_write_ptr, want_read);
                if (bytes_read <= 0) {
                    delay(RETRY_DELAY);
                    bytes_read = stream.readBytes(buf_write_ptr, want_read);
                    if (bytes_read <= 0) {
                        break;
                    }
                }
                content_length -= bytes_read;
                buf_write_ptr += bytes_read;
            }
            callback(buf_write_ptr - buffer);
            buf_write_ptr = buffer;
        }
        content_length = http.getSize();
        http.end();
        return content_length;

    } else if (content_type == "application/json") {
        String body = http.getString();
        http.end();
        JsonDocument doc;
        deserializeJson(doc, body);
        report_error(doc["err_msg"].as<String>());
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

size_t cloudSynth(uint8_t *buffer, size_t size, void (*callback)(size_t bytes_read), String const &text, String const &options)
{
    return speech_synth(token, buffer, size, callback, text, options);
}
