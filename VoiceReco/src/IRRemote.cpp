#include "IRRemote.h"
#include "AIChat.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Gree.h>

#define IR_LED_PIN GPIO_NUM_4

static IRGreeAC *ac;

static const char *greeBoolNames[] = {
    "off",
    "on",
};

static const char *greeModeNames[] = {
    "auto",
    "cool",
    "dry",
    "fan",
    "heat",
    "econo",
};

static const char *greeFanNames[] = {
    "auto",
    "min",
    "med",
    "max",
};

static const char *greeSwingNames[] = {
    "lastpos",
    "auto",
    "up",
    "middle_up",
    "middle",
    "middle_down",
    "down",
    "down_auto",
    "middle_auto",
    "up_auto",
};

static const char *greeSwingHNames[] = {
    "off",
    "auto",
    "max_left",
    "left",
    "middle",
    "right",
    "max_right",
};

String get_ac_state(JsonDocument const &arguments)
{
    JsonDocument result;

    result["power"] = ac->getPower();
    result["mode"] = greeModeNames[ac->getMode()];

    result["temp"] = ac->getTemp();
    result["fan"] = ac->getFan();

    result["sleep"] = ac->getSleep();
    result["turbo"] = ac->getTurbo();
    result["light"] = ac->getLight();

    // result["swing_v"] = greeSwingNames[ac->getSwingVerticalAuto()];
    // result["swing_h"] = greeSwingHNames[ac->getSwingHorizontal()];

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

String set_ac_state(JsonDocument const &arguments)
{
    if (!arguments["power"].isNull()) {
        ac->setPower(arguments["power"]);
    }
    if (!arguments["mode"].isNull()) {
        for (int i = 0; i < sizeof greeModeNames / sizeof greeModeNames[0]; ++i) {
            if (arguments["mode"] == greeModeNames[i]) {
                ac->setMode(i);
            }
        }
    }

    if (!arguments["temp"].isNull()) {
        if (ac->getMode() == kGreeAuto) {
            ac->setMode(kGreeCool);
        }
        ac->setTemp(arguments["temp"]);
    }
    if (!arguments["fan"].isNull()) {
        ac->setFan(arguments["fan"]);
    }

    if (!arguments["sleep"].isNull()) {
        ac->setSleep(arguments["sleep"]);
    }
    if (!arguments["turbo"].isNull()) {
        ac->setTurbo(arguments["turbo"]);
    }
    if (!arguments["light"].isNull()) {
        ac->setLight(arguments["light"]);
    }

    ac->send();
    return get_ac_state(arguments);
}

String change_ac_temp(JsonDocument const &arguments)
{
    if (!arguments["temp_delta"].isNull()) {
        int dt = arguments["temp_delta"];
        ac->setPower(true);
        if (ac->getMode() == kGreeAuto) {
            ac->setMode(kGreeCool);
        }
        ac->setTemp(ac->getTemp() + dt);
        ac->send();
    }

    return get_ac_state(arguments);
}

void remoteSetup()
{
    ac = new IRGreeAC(IR_LED_PIN);
    ac->begin();
    ac->calibrate();

    registerTool(Tool{
        .name = "get_ac_state",
        .descrption = "获取空调状态",
        .parameters = {
        },
        .callback = get_ac_state,
    });
    registerTool(Tool{
        .name = "set_ac_state",
        .descrption = "设置空调状态（无需指定所有字段，未指定的字段会自动维持原状态）",
        .parameters = {
            {
                .name = "power",
                .descrption = "空调开启：true=启动，false=关闭",
                .type = "boolean",
            },
            {
                .name = "mode",
                .descrption = "空调模式：cool=制冷，heat=制热",
                .type = "string",
            },
            {
                .name = "temp",
                .descrption = "空调目标温度（摄氏度）",
                .type = "integer",
            },
            {
                .name = "fan",
                .descrption = "空调风力：0=自动，1=弱，2=中，3=强",
                .type = "integer",
            },
            {
                .name = "sleep",
                .descrption = "睡眠模式",
                .type = "boolean",
            },
            {
                .name = "turbo",
                .descrption = "强劲模式",
                .type = "boolean",
            },
            {
                .name = "turbo",
                .descrption = "强劲模式",
                .type = "boolean",
            },
            {
                .name = "light",
                .descrption = "空调灯光",
                .type = "boolean",
            },
        },
        .callback = set_ac_state,
    });
    registerTool(Tool{
        .name = "change_ac_temp",
        .descrption = "增量修改空调温度，在现有温度基础上下变化",
        .parameters = {
            {
                .name = "temp_delta",
                .descrption = "空调温度增量（摄氏度）：+1=调高1度，-1=调低1度",
                .type = "integer",
            },
        },
        .callback = change_ac_temp,
    });
}
