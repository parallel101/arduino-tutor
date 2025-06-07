#include <Arduino.h>
#include "Assistant.h"
#include "AIChat.h"
#include <esp_system.h>
#include <esp_timer.h>

static int assistantSpeed = 5;
static int assistantVolume = 2;
static int assistantTimbre = 0;
static int assistantTimeout = 60;
static volatile bool assistantSleep = false;
static esp_timer_handle_t sleepTimer;

int getAssistantVolume()
{
    return assistantVolume;
}

void on_sleep_timeout(void *)
{
    assistantSleep = true;
}

void resetAssistantSleep()
{
    printf("Assistant awake.\n");
    esp_timer_stop(sleepTimer);
    if (assistantTimeout > 0) {
        uint64_t us = assistantTimeout * UINT64_C(1'000'000);
        ESP_ERROR_CHECK(esp_timer_start_once(sleepTimer, us));
    }
}

void updateAssistantSleep()
{
    if (assistantSleep) {
        printf("Assistant sleep.\n");
        aiChatSetLevel(0);
        aiChatReset();
        assistantSleep = false;
    }
}

String getSynthOptions()
{
    return String("&vol=") + assistantVolume + "&per=" + assistantTimbre + "&spd=" + assistantSpeed;
}

static String set_assistant_voice(JsonDocument const &arguments)
{
    if (!arguments["volume"].isNull()) {
        assistantVolume = arguments["volume"];
    }
    if (!arguments["person"].isNull()) {
        assistantTimbre = arguments["person"];
    }
    if (!arguments["speed"].isNull()) {
        assistantSpeed = arguments["speed"];
    }
    return R"({"status": ")" + getSynthOptions() + R"("})";
}

static String set_assistant_level(JsonDocument const &arguments)
{
    aiChatSetLevel(arguments["level"]);
    return R"({"status": "OK"})";
}

static String shutdown_assistant(JsonDocument const &arguments)
{
    aiChatSetLevel(0);
    aiChatReset();
    return "SHUTDOWN";
}

void assistantSetup()
{
    static const esp_timer_create_args_t timerArgs = {
        .callback = on_sleep_timeout,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sleep_timeout",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &sleepTimer));

    registerTool(Tool{
        .name = "set_assistant_voice",
        .descrption = "语音助手播报设置",
        .parameters = {
            {
                .name = "volume",
                .descrption = "音量：0~9（初始为2）",
                .type = "integer",
            },
            {
                .name = "person",
                .descrption = "音色：0=无感情，4176=成熟男，4192=温柔男，4288=温柔女，6562=御姐，6543=萌妹",
                .type = "integer",
            },
            {
                .name = "speed",
                .descrption = "语速：0~15（初始为5）",
                .type = "integer",
            },
        },
        .callback = set_assistant_voice,
    });
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
    registerTool({
        .name = "shutdown_assistant",
        .descrption = "语音助手关机",
        .parameters = {
        },
        .callback = shutdown_assistant,
    });
    // registerTool(Tool{
    //     .name = "set_assistant_sleep",
    //     .descrption = "设置语音助手休眠前的等待时间，超时未使用后会自动休眠",
    //     .parameters = {
    //         {
    //             .name = "timeout",
    //             .descrption = "休眠等待时间（秒）：0~600（初始为60）",
    //             .type = "integer",
    //         },
    //     },
    //     .callback = set_assistant_sleep,
    // });
}
