#include "AIChat.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "CACert.h"
#include "GlobalHTTP.h"
#include "secrets.h"
#include "report_error.h"

#define MAX_RETRIES 5
#define MAX_TOOL_CALLS 3
#define RETRY_DELAY 100
static const char SYSTEM_INSTRUCTION[] =
R"(你是一个家庭语音助手，昵称小智。
你的职责是帮助用户解决生活问题，可以操控用户家中的电器设备，解决用户需求。

你的输出严格遵守以下规则：
1. 你的输入来自用户的语音，由于语音识别的局限，输入文本可能存在错别字，请在心里自行纠正，不必告诉用户。
2. 你的输出将被合成为语音后通过扬声器朗读播报。
3. 你的输出文本必须符合口语的用于习惯。
4. 确保你输出的每个字符，都能被语音合成引擎朗读出来，不要包含任何无法朗读的东西。
5. 不要包含 Markdown 助记符，不要包含程序代码，不要包含 Emoji，但可以包含标点符号。
6. 回答不得超过 50 字。
7. 确保回答简短，直达主题，快速解决用户问题，避免废话。
8. 如果用户请求进行某种操作，请通过 Tool Call 对物联网设备进行操作，解决用户的需求。
9. 由于内存限制，你没有历史对话记忆。你需要在一轮对话中解决问题，不会有第二轮对话。
10. 涉及设备操作时，不用询问用户确认，直接执行 Tool Call 即可。

用户偏好设置：
1. 用户昵称：小彭老师
2. 用户爱好：嵌入式编程
3. 偏好对话风格：科技并且带着趣味
4. 语音助手硬件：ESP32C3 微型开发板)";

static const char *roleName(Role role)
{
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

static std::vector<Tool> tools;

void registerTool(Tool tool)
{
    tools.push_back(std::move(tool));
}

template <class Tools>
static void getAITools(Tools toolsArr)
{
    for (int i = 0; i < tools.size(); ++i) {
        toolsArr[i]["type"] = "function";
        toolsArr[i]["function"]["name"] = tools[i].name;
        toolsArr[i]["function"]["descrption"] = tools[i].descrption;
        toolsArr[i]["function"]["parameters"]["type"] = "object";
        for (int j = 0; j < tools[i].parameters.size(); ++j) {
            toolsArr[i]["function"]["parameters"]["properties"][tools[i].parameters[j].name]["descrption"] = tools[i].parameters[j].descrption;
            toolsArr[i]["function"]["parameters"]["properties"][tools[i].parameters[j].name]["type"] = tools[i].parameters[j].type;
        }
    }
}

std::vector<Message> messages;

void aiChatReset()
{
    messages.push_back({
        .role = Role::System,
        .content = SYSTEM_INSTRUCTION,
    });
}

static String aiChatComplete(AIOptions const &options)
{
    for (int ntc = 0; ntc < MAX_TOOL_CALLS; ++ntc) {
        static const char url[] = "https://qianfan.baidubce.com/v2/chat/completions";
        http.begin(url, certBaiduCom);
        http.setTimeout(20 * 1000);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " BAIDU_BCE_API_KEY);

        {
            JsonDocument doc;
            for (size_t i = 0; i != messages.size(); ++i) {
                doc["messages"][i]["role"] = roleName(messages[i].role);
                doc["messages"][i]["content"] = messages[i].content;
                if (!messages[i].reasoningContent.isEmpty()) {
                    doc["messages"][i]["reasoning_content"] = messages[i].reasoningContent;
                }
                if (!messages[i].name.isEmpty()) {
                    doc["messages"][i]["name"] = messages[i].name;
                }
                if (!messages[i].toolCallId.isEmpty()) {
                    doc["messages"][i]["tool_call_id"] = messages[i].toolCallId;
                }
                if (!messages[i].toolCalls.empty()) {
                    for (int j = 0; j < messages[i].toolCalls.size(); ++j) {
                        doc["messages"][i]["tool_calls"][j]["id"] = messages[i].toolCalls[j].id;
                        doc["messages"][i]["tool_calls"][j]["type"] = "function";
                        doc["messages"][i]["tool_calls"][j]["function"]["argments"] = messages[i].toolCalls[j].functionArgments;
                        doc["messages"][i]["tool_calls"][j]["function"]["name"] = messages[i].toolCalls[j].functionName;
                    }
                }
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
            getAITools(doc["tools"]);
            doc["tool_choice"] = "auto";

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
                report_error(http.getString());
                http.end();
                return "";
            }
        }

        String body = http.getString();
        http.end();
        Serial.println(body);

        JsonDocument doc;
        deserializeJson(doc, body);
        if (doc["choices"].isNull()) {
            report_error(body);
            return "";
        }

        JsonObject response = doc["choices"][0]["message"];
        if (response["role"] != "assistant") {
            report_error(body);
            return "";
        }

        Message assistantMessage = {
            .role = Role::Assistant,
            .content = response["content"],
            .reasoningContent = response["reasoning_content"].isNull() ? String() : response["reasoning_content"],
        };
        for (int i = 0; i < response["tool_calls"].size(); ++i) {
            assistantMessage.toolCalls.push_back({
                .id = response["tool_calls"][i]["id"],
                .functionName = response["tool_calls"][i]["function"]["name"],
                .functionArgments = response["tool_calls"][i]["function"]["arguments"],
            });
        }
        messages.push_back(std::move(assistantMessage));

        if (!response["tool_calls"].isNull() && response["tool_calls"].size() > 0) {
            std::vector<String> results;
            for (int i = 0; i < response["tool_calls"].size(); ++i) {
                String toolCallId = response["tool_calls"][i]["id"];
                String functionName = response["tool_calls"][i]["function"]["name"];
                String functionArgs = response["tool_calls"][i]["function"]["arguments"];
                JsonDocument argsDoc;
                deserializeJson(argsDoc, functionArgs);
                int toolIndex = -1;
                for (int j = 0; j < tools.size(); ++j) {
                    if (functionName == tools[j].name) {
                        toolIndex = j;
                    }
                }
                String toolResponse;
                if (toolIndex != -1) {
                    toolResponse = tools[toolIndex].callback(argsDoc);
                } else {
                    toolResponse = R"({"error": "tool not found"})";
                }
                Serial.println(toolResponse);
                messages.push_back({
                    .role = Role::Tool,
                    .content = std::move(toolResponse),
                    .name = functionName,
                    .toolCallId = toolCallId,
                });
            }
            continue;
        }

        Serial.println(response["content"].as<String>());
        return response["content"];
    }
    return "";
}

String aiChat(String const &prompt, AIOptions const &options)
{
    if (messages.empty()) {
        aiChatReset();
    }
    messages.push_back({
        .role = Role::User,
        .content = prompt,
    });
    return aiChatComplete(options);
}
