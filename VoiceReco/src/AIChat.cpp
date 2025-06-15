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
R"(你是一个家庭语音助手，昵称)" NICK_NAME R"(。
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

static std::vector<Tool> tools;
static std::vector<Message> messages;
static AIOptions aiOptions;

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

static int aiLevel;

int aiChatGetLevel()
{
    return aiLevel;
}

void aiChatSetLevel(int level)
{
    aiLevel = level;
    switch (level) {
    case 0:
        aiOptions.model = nullptr;
        break;
    case 1:
        aiOptions.model = "deepseek-v3";
        break;
    case 2:
        aiOptions.model = "deepseek-r1";
        break;
    }
}

void aiChatSetup()
{
    aiLevel = 0;
    aiChatReset();
}

void aiChatReset()
{
    messages.clear();
    messages.push_back({
        .role = Role::System,
        .content = SYSTEM_INSTRUCTION,
    });
}

int findTool(String const &name)
{
    for (int i = 0; i < tools.size(); ++i) {
        if (name == tools[i].name) {
            return i;
        }
    }
    return -1;
}

static String aiChatComplete()
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

            doc["model"] = aiOptions.model;
            doc["stream"] = false;
            doc["user_id"] = BAIDU_CUID;
            doc["temperature"] = aiOptions.temperature;
            if (aiOptions.seed != -1) {
                doc["seed"] = aiOptions.seed;
            }
            if (aiOptions.max_tokens != -1) {
                doc["max_tokens"] = aiOptions.max_tokens;
            }
            getAITools(doc["tools"]);
            doc["tool_choice"] = "auto";

            String jsonStr;
            serializeJson(doc, jsonStr);
            // printf("request json: %s\n", jsonStr.c_str());

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
        // printf("response json: %s\n", body.c_str());

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
                printf("calling %s: %s\n", functionName.c_str(), functionArgs.c_str());
                JsonDocument argsDoc;
                deserializeJson(argsDoc, functionArgs);
                int toolIndex = findTool(functionName);
                String toolResponse;
                if (toolIndex != -1) {
                    toolResponse = tools[toolIndex].callback(argsDoc);
                } else {
                    toolResponse = R"({"error": "tool not found"})";
                }
                printf("tool response: %s\n", toolResponse.c_str());
                if (toolResponse == "SHUTDOWN") {
                    return "";
                }
                messages.push_back({
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
            deserializeJson(resDoc, tools[toolIndex].callback(argsDoc));
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
            deserializeJson(resDoc, tools[toolIndex].callback(argsDoc));
            bool power = resDoc["power"];
            String mode = resDoc["mode"];
            int temperature = resDoc["temp"];
            int fan = resDoc["fan"];
            int sleep = resDoc["sleep"];
            int turbo = resDoc["turbo"];
            int light = resDoc["light"];
            String result;
            if (power) {
                result += "当前温度";
                result += temperature;
                result += "度，";
                if (mode == "auto") {
                    result += "自动";
                } else if (mode == "cool") {
                    result += "制冷";
                } else if (mode == "fan") {
                    result += "吹风";
                } else if (mode == "dry") {
                    result += "干燥";
                } else if (mode == "hot") {
                    result += "制热";
                } else if (mode == "econo") {
                    result += "环保";
                } else {
                    result += mode;
                }
                result += "模式，风速";
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

    } else if (prompt == "时间") {
        JsonDocument argsDoc;
        int toolIndex = findTool("get_time");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            deserializeJson(resDoc, tools[toolIndex].callback(argsDoc));
            String result = resDoc["time"];
            return result;
        }

    } else if (prompt == "开空调" || prompt == "关空调") {
        JsonDocument argsDoc;
        argsDoc["power"] = prompt == "开空调";
        int toolIndex = findTool("set_ac_state");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
            return "已" + prompt;
        }

    } else if (prompt == "空调提高") {
        JsonDocument argsDoc;
        argsDoc["temp_delta"] = +1;
        int toolIndex = findTool("change_ac_temp");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
            return "空调已调高一度";
        }

    } else if (prompt == "空调降低") {
        JsonDocument argsDoc;
        argsDoc["temp_delta"] = -1;
        int toolIndex = findTool("change_ac_temp");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
            return "空调已调低一度";
        }

    } else if (prompt == "开灯" || prompt == "关灯") {
        JsonDocument argsDoc;
        argsDoc["power"] = prompt == "开灯";
        int toolIndex = findTool("set_light_state");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
            return "已" + prompt;
        }

    } else if (prompt == "指令模式" || prompt == "智能模式" || prompt == "思考模式") {
        JsonDocument argsDoc;
        argsDoc["level"] = prompt == "指令模式" ? 0 : prompt == "思考模式" ? 2 : 1;
        int toolIndex = findTool("set_assistant_level");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
            return "进入" + prompt;
        }

    } else if (prompt == NICK_NAME || prompt == NICK_NAME_TYPO) {
        JsonDocument argsDoc;
        argsDoc["min_level"] = 1;
        int toolIndex = findTool("set_assistant_level");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
        }
        return "怎么啦？";

    } else if (prompt.startsWith(NICK_NAME)) {
        JsonDocument argsDoc;
        argsDoc["min_level"] = 1;
        int toolIndex = findTool("set_assistant_level");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
        }
        return "";

    } else if (prompt == "关机") {
        JsonDocument argsDoc;
        int toolIndex = findTool("shutdown_assistant");
        if (toolIndex != -1) {
            JsonDocument resDoc;
            tools[toolIndex].callback(argsDoc);
        }
        return "已关机";
    }
    return "";
}

String aiChat(String const &prompt)
{
    String instructReply = instructiveChat(prompt);
    if (!instructReply.isEmpty()) {
        return instructReply;
    }
    if (aiOptions.model == nullptr) {
        printf("instruction not understood\n");
        return "";
    }

    if (messages.empty()) {
        aiChatReset();
    }
    messages.push_back({
        .role = Role::User,
        .content = prompt,
    });
    return aiChatComplete();
}

AIOptions &aiGetOptions()
{
    return aiOptions;
}
