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
1. 你的输入来自用户的语音，由于语音识别的局限，输入文本可能有错别字，请在心里纠正，不必告诉用户。
2. 你的输出将被合成为语音后通过扬声器朗读播报。
3. 你的输出文本必须符合口语的用于习惯。
4. 确保你输出的每个字符，都能被语音合成引擎朗读出来，不要包含任何无法朗读的东西。
5. 不要包含 Markdown 助记符，不要包含程序代码，不要包含 Emoji，但可以包含标点符号。
6. 回答不得超过 50 字，简洁明了，直接解决用户问题，避免废话。
7. 若用户请求进行某种操作，请通过 Tool Call 对物联网设备进行操作，解决用户需求。
8. 若用户请求的操作在 Tool 列表中不存在，请直接告诉用户暂不支持。

用户偏好设置：
1. 用户昵称：小彭老师
2. 用户爱好：嵌入式编程
3. 偏好对话风格：科技并且带着趣味
4. 语音助手硬件：ESP32 微型开发板)";

static AIState *aiState;

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

void registerTool(Tool tool)
{
    aiState->tools.push_back(std::move(tool));
}

template <class Tools>
static void getAITools(Tools toolsArr)
{
    for (int i = 0; i < aiState->tools.size(); ++i) {
        toolsArr[i]["type"] = "function";
        toolsArr[i]["function"]["name"] = aiState->tools[i].name;
        toolsArr[i]["function"]["descrption"] = aiState->tools[i].descrption;
        toolsArr[i]["function"]["parameters"]["type"] = "object";
        for (int j = 0; j < aiState->tools[i].parameters.size(); ++j) {
            toolsArr[i]["function"]["parameters"]["properties"][aiState->tools[i].parameters[j].name]["descrption"] = aiState->tools[i].parameters[j].descrption;
            toolsArr[i]["function"]["parameters"]["properties"][aiState->tools[i].parameters[j].name]["type"] = aiState->tools[i].parameters[j].type;
        }
    }
}

static String set_assistant_level(JsonDocument const &arguments) {
    int level = arguments["level"];
    switch (level) {
    case 0:
        aiState->options.model = nullptr;
        break;
    case 1:
        aiState->options.model = "deepseek-v3";
        break;
    case 2:
        aiState->options.model = "deepseek-r1";
        break;
    }
    return R"({"status": "OK"})";
}

void aiChatSetup()
{
    aiState = new AIState;
    registerTool({
        .name = "set_assistant_level",
        .descrption = "设置助手智能等级",
        .parameters = {
            {
                .name = "level",
                .descrption = "智能等级：0=指令模式（快），1=智能模式（默认），2=思考模式（慢）",
                .type = "integer",
            },
        },
        .callback = set_assistant_level,
    });
    aiChatReset();
}

void aiChatReset()
{
    aiState->messages.clear();
    aiState->messages.push_back({
        .role = Role::System,
        .content = SYSTEM_INSTRUCTION,
    });
}

int findTool(String const &name)
{
    for (int i = 0; i < aiState->tools.size(); ++i) {
        if (name == aiState->tools[i].name) {
            return i;
        }
    }
    return -1;
}

static String aiChatComplete()
{
    for (int ntc = 0; ntc < MAX_TOOL_CALLS; ++ntc) {
        static const char url[] = "https://qianfan.baidubce.com/v2/chat/completions";
        http->begin(url, certBaiduCom);
        http->setTimeout(20 * 1000);
        http->addHeader("Content-Type", "application/json");
        http->addHeader("Authorization", "Bearer " BAIDU_BCE_API_KEY);

        {
            JsonDocument doc;
            for (size_t i = 0; i != aiState->messages.size(); ++i) {
                doc["aiState->messages"][i]["role"] = roleName(aiState->messages[i].role);
                doc["aiState->messages"][i]["content"] = aiState->messages[i].content;
                if (!aiState->messages[i].reasoningContent.isEmpty()) {
                    doc["aiState->messages"][i]["reasoning_content"] = aiState->messages[i].reasoningContent;
                }
                if (!aiState->messages[i].name.isEmpty()) {
                    doc["aiState->messages"][i]["name"] = aiState->messages[i].name;
                }
                if (!aiState->messages[i].toolCallId.isEmpty()) {
                    doc["aiState->messages"][i]["tool_call_id"] = aiState->messages[i].toolCallId;
                }
                if (!aiState->messages[i].toolCalls.empty()) {
                    for (int j = 0; j < aiState->messages[i].toolCalls.size(); ++j) {
                        doc["aiState->messages"][i]["tool_calls"][j]["id"] = aiState->messages[i].toolCalls[j].id;
                        doc["aiState->messages"][i]["tool_calls"][j]["type"] = "function";
                        doc["aiState->messages"][i]["tool_calls"][j]["function"]["argments"] = aiState->messages[i].toolCalls[j].functionArgments;
                        doc["aiState->messages"][i]["tool_calls"][j]["function"]["name"] = aiState->messages[i].toolCalls[j].functionName;
                    }
                }
            }

            doc["model"] = aiState->options.model;
            doc["stream"] = false;
            doc["user_id"] = BAIDU_CUID;
            doc["temperature"] = aiState->options.temperature;
            if (aiState->options.seed != -1) {
                doc["seed"] = aiState->options.seed;
            }
            if (aiState->options.max_tokens != -1) {
                doc["max_tokens"] = aiState->options.max_tokens;
            }
            getAITools(doc["aiState->tools"]);
            doc["tool_choice"] = "auto";

            String jsonStr;
            serializeJson(doc, jsonStr);
            printf("request json: %s\n", jsonStr.c_str());

            int code;
            for (int tries = 0; tries != MAX_RETRIES; ++tries) {
                code = http->POST(jsonStr);
                if (code >= 0) {
                    break;
                }
                delay(RETRY_DELAY);
            }
            if (code != 200) {
                report_error(http->getString());
                http->end();
                return "";
            }
        }

        String body = http->getString();
        http->end();
        printf("response json: %s\n", body.c_str());

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
        aiState->messages.push_back(std::move(assistantMessage));

        if (!response["tool_calls"].isNull() && response["tool_calls"].size() > 0) {
            std::vector<String> results;
            for (int i = 0; i < response["tool_calls"].size(); ++i) {
                String toolCallId = response["tool_calls"][i]["id"];
                String functionName = response["tool_calls"][i]["function"]["name"];
                String functionArgs = response["tool_calls"][i]["function"]["arguments"];
                JsonDocument argsDoc;
                deserializeJson(argsDoc, functionArgs);
                int toolIndex = findTool(functionName);
                String toolResponse;
                if (toolIndex != -1) {
                    toolResponse = aiState->tools[toolIndex].callback(argsDoc);
                } else {
                    toolResponse = R"({"error": "tool not found"})";
                }
                printf("tool response: %s\n", toolResponse.c_str());
                aiState->messages.push_back({
                    .role = Role::Tool,
                    .content = std::move(toolResponse),
                    .name = functionName,
                    .toolCallId = toolCallId,
                });
            }
            continue;
        }

        printf("content: %s\n", response["content"].as<String>().c_str());
        return response["content"];
    }
    return "";
}

String instructiveChat(String prompt)
{
    prompt.replace("。", "");
    if (prompt == "温度" || prompt == "湿度") {
        JsonDocument argsDoc;
        int toolIndex = findTool("get_temperature");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            deserializeJson(resDoc, aiState->tools[toolIndex].callback(argsDoc));
            float temperature = resDoc["temperature"];
            float humidity = resDoc["humidity"];
            String result;
            result += "当前温度";
            result += temperature;
            result += "度";
            result += "，湿度";
            result += humidity;
            result += "%";
            return result;
        }

    } else if (prompt == "空调状态") {
        JsonDocument argsDoc;
        int toolIndex = findTool("get_ac_state");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            deserializeJson(resDoc, aiState->tools[toolIndex].callback(argsDoc));
            bool power = resDoc["power"];
            float temperature = resDoc["temp"];
            int fan = resDoc["fan"];
            int sleep = resDoc["sleep"];
            int turbo = resDoc["turbo"];
            int light = resDoc["light"];
            String result;
            if (power) {
                result += "当前温度";
                result += temperature;
                result += "度";
                result += "，风速";
                if (fan == 0) {
                    result += "自动";
                } else {
                    result += fan;
                    result += "级";
                }
                if (sleep) {
                    result += "，睡眠模式";
                }
                if (turbo) {
                    result += "，强劲模式";
                }
                if (light) {
                    result += "，灯光开启";
                }
            } else {
                result += "当前空调关闭";
            }
            return result;
        }

    } else if (prompt == "开空调" || prompt == "关空调") {
        JsonDocument argsDoc;
        argsDoc["power"] = prompt == "打开空调";
        int toolIndex = findTool("set_ac_state");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            aiState->tools[toolIndex].callback(argsDoc);
            return "已" + prompt;
        }

    } else if (prompt == "开灯" || prompt == "关灯") {
        JsonDocument argsDoc;
        argsDoc["power"] = prompt == "开灯";
        int toolIndex = findTool("set_light_state");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            aiState->tools[toolIndex].callback(argsDoc);
            return "已" + prompt;
        }

    } else if (prompt == "指令模式" || prompt == "智能模式" || prompt == "思考模式") {
        JsonDocument argsDoc;
        argsDoc["level"] = prompt == "指令模式" ? 0 : prompt == "思考模式" ? 2 : 1;
        int toolIndex = findTool("set_assistant_level");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            aiState->tools[toolIndex].callback(argsDoc);
            return "进入" + prompt;
        }
    }
    return "";
}

String aiChat(String const &prompt)
{
    String instructReply = instructiveChat(prompt);
    if (!instructReply.isEmpty()) {
        return instructReply;
    }
    if (aiState->options.model == nullptr) {
        printf("instruction not understood\n");
        return "";
    }

    if (aiState->messages.empty()) {
        aiChatReset();
    }
    aiState->messages.push_back({
        .role = Role::User,
        .content = prompt,
    });
    return aiChatComplete();
}

AIOptions &aiGetOptions()
{
    return aiState->options;
}
