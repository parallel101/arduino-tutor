#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

enum class Role
{
    System,
    User,
    Assistant,
    Tool,
};

struct Message
{
    struct ToolCall {
        String id;
        String functionName;
        String functionArgments;
    };

    Role role;
    String content;
    String reasoningContent;
    String name;
    String toolCallId;
    std::vector<ToolCall> toolCalls;
};

struct Tool
{
    struct Parameter
    {
        const char *name;
        const char *descrption;
        const char *type;
    };

    const char *name;
    const char *descrption;
    std::vector<Parameter> parameters;
    String (*callback)(JsonDocument const &);
};

struct AIOptions
{
    const char *model = nullptr;
    float temperature = 0.1;
    int32_t seed = -1;
    int32_t max_tokens = 128;
};

void aiChatSetup();
void registerTool(Tool tool);
void aiChatReset();
String aiChat(String const &prompt);
AIOptions &aiGetOptions();
