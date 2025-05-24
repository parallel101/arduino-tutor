#include "AIChat.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "secrets.h"
#include "report_error.h"

#define MAX_RETRIES 5
#define RETRY_DELAY 100
#define MODEL_NAME "ernie-4.5-turbo-32k"
static const char SYSTEM_INSTRUCTION[] =
R"(你是一个家庭语音助手，昵称小智。
你的职责是帮助用户解决生活问题，你可以操控用户家中的电器设备，解决用户需求。

你的输出严格遵守以下规则：
1. 你的输入来自用户的语音，由于语音识别的局限，输入文本可能存在错别字，请在心里自行纠正，不必告诉用户。
2. 你的输出将被合成为语音后通过扬声器朗读播报。
3. 你的输出文本必须符合口语的用于习惯。
4. 确保你输出的每个字符，都能被语音合成引擎朗读出来，不要包含任何无法朗读的东西。
5. 不要包含 Markdown 助记符，不要包含程序代码，不要包含 Emoji，但可以包含标点符号。
6. 回答不得超过 50 字。
7. 确保回答简短，直达主题，快速解决用户问题，避免废话。
8. 如果用户请求进行某种操作，请通过 Tool Call 对物联网设备进行操作，解决用户的需求。
9. 如有必要，可以在 Tool Call 后配上简短的语音提示，不超过 10 个字。
10. 由于内存限制，你没有历史对话记忆。你需要在一轮对话中解决问题，不会有第二轮对话。
11. 涉及设备操作时，不用询问用户确认，直接执行 Tool Call 即可。

用户偏好设置：
1. 用户昵称：小彭老师
2. 用户爱好：嵌入式编程
3. 偏好对话风格：科技并且带着趣味
4. 语音助手硬件：ESP32C3 微型开发板
5. 已知物联网设备：暂无，需要用户购买

对话案例：
User: 什么是电灯？
Assistant: 就是一种用电来照亮你房间的电器啦！
User: 打开电灯。
Assistant: （使用 Tool Call 打开电灯）已为您打开电灯。)";

static const char *roleName(Role role) {
    switch (role) {
        case Role::System:
            return "system";
        case Role::User:
            return "user";
        case Role::Assistant:
            return "assistant";
        case Role::Tool:
            return "tool";
        default:
            return "";
    }
}

String aiChat(String const &prompt, AIOptions const &options)
{
    Message messages[2];
    messages[0].role = Role::System;
    messages[0].content = SYSTEM_INSTRUCTION;
    messages[1].role = Role::User;
    messages[1].content = prompt;
    return aiChat(messages, sizeof messages / sizeof messages[0], options);
}

String aiChat(Message *messages, size_t num_messages, AIOptions const &options)
{
    static const char url[] = "http://qianfan.baidubce.com/v2/chat/completions";
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " BAIDU_BCE_API_KEY);

    {
        JsonDocument doc;
        for (size_t i = 0; i != num_messages; ++i) {
            doc["messages"][i]["role"] = roleName(messages[i].role);
            doc["messages"][i]["content"] = messages[i].content;
        }

        doc["model"] = options.model;
        doc["stream"] = false;
        doc["user_id"] = BAIDU_CUID;
        doc["temperature"] = options.temperature;
        if (options.seed != -1) {
            doc["seed"] = options.seed;
        }
        if (options.max_tokens != -1) {
            doc["max_tokens"] = options.max_tokens;
        }

        String jsonStr;
        serializeJson(doc, jsonStr);
        Serial.println(jsonStr);

        int code;
        for (int tries = 0; tries != MAX_RETRIES; ++tries) {
            code = http.POST(jsonStr);
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
    }

    String body = http.getString();
    http.end();
    Serial.println(body);

    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println("Deserialized!");
    if (doc["choices"].isNull()) {
        report_error(body);
        return "";
    }

    Serial.println("Choiced!");
    JsonObject response = doc["choices"][0]["message"];
    if (response["role"] != "assistant") {
        report_error(body);
        return "";
    }
    if (!response["tool_calls"].isNull()) {
        report_error("tool calls not supported yet");
        return "";
    }

    Serial.println("Contented!");
    Serial.println(response["content"].as<String>());
    return response["content"];
}
