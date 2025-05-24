#pragma once

#include <Arduino.h>

enum class Role {
    System,
    User,
    Assistant,
    Tool,
};

struct Message
{
    Role role;
    String content;
};

struct AIOptions
{
    // const char *model = "qianfan-agent-intent-32k";
    const char *model = "ernie-4.5-turbo-32k";
    float temperature = 0.1;
    int32_t seed = -1;
    int32_t max_tokens = 80;
};

String aiChat(String const &prompt, AIOptions const &options = {});
String aiChat(Message *messages, size_t num_messages, AIOptions const &options = {});
