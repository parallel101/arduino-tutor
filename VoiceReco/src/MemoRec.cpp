#include "MemoRec.h"
#include "AIChat.h"
#include <Arduino.h>
#include <NTPClientExt.hpp>
#include <deque>

#define DHT_PIN GPIO_NUM_13

static std::deque<Memo> memos;

NTPClientExt timeClient("ntp.aliyun.com", +8, 10, false);

static String get_time(JsonDocument const &arguments) {
    JsonDocument result;
    timeClient.update();
    result["time"] = timeClient.getFormattedDateTime();

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

static String write_memo(JsonDocument const &arguments) {
    if (arguments["content"].isNull()) {
        return R"({"error": "no content"})";
    }
    timeClient.update();
    String time = timeClient.getFormattedDateTime();
    memos.push_back({
        .content = arguments["content"],
        .time = time,
    });
    return R"({"status": "OK"})";
}

static String fetch_memo(JsonDocument const &arguments) {
    JsonDocument result;
    if (memos.empty()) {
        result["status"] = "no memos yet";
    } else {
        Memo *memo;
        if (arguments["mode"] == "earliest") {
            result["mode"] = "earliest";
            memo = &memos.front();
        } else {
            result["mode"] = "latest";
            memo = &memos.back();
        }
        result["content"] = memo->content;
        result["time"] = memo->time;
    }

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

static String delete_memo(JsonDocument const &arguments) {
    JsonDocument result;
    if (memos.empty()) {
        result["status"] = "no memos yet";
    } else {
        Memo memo;
        if (arguments["mode"] == "earliest") {
            result["mode"] = "earliest";
            memo = memos.front();
            memos.pop_front();
        } else {
            result["mode"] = "latest";
            memo = memos.back();
            memos.pop_back();
        }
        result["content"] = memo.content;
        result["time"] = memo.time;
    }

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

static String fetch_all_memo(JsonDocument const &arguments) {
    JsonDocument result;
    if (memos.empty()) {
        result["status"] = "no memos yet";
    } else {
        for (int i = 0; i < memos.size(); ++i) {
            result[i]["content"] = memos[i].content;
            result[i]["time"] = memos[i].time;
        }
    }

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

static String delete_all_memo(JsonDocument const &arguments) {
    JsonDocument result;
    if (memos.empty()) {
        result["status"] = "no memos yet";
    } else {
        result["deleted_count"] = memos.size();
        memos.clear();
    }

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

static String clear_memo(JsonDocument const &arguments) {
    JsonDocument result;
    if (memos.empty()) {
        result["status"] = "no memos yet";
    } else {
        for (int i = 0; i < memos.size(); ++i) {
            result[i]["content"] = memos[i].content;
            result[i]["time"] = memos[i].time;
        }
    }

    String resultStr;
    serializeJson(result, resultStr);
    return resultStr;
}

void memoSetup() {
    timeClient.begin();
    timeClient.setTimeOffset(3600);
    timeClient.update();

    registerTool(Tool{
        .name = "get_time",
        .descrption = "查询当前日期和时间",
        .parameters = {
        },
        .callback = get_time,
    });
    registerTool(Tool{
        .name = "write_memo",
        .descrption = "添加一条备忘录",
        .parameters = {
            {
                .name = "content",
                .descrption = "备忘录内容",
                .type = "string",
            },
        },
        .callback = write_memo,
    });
    registerTool(Tool{
        .name = "fetch_memo",
        .descrption = "读取一条备忘录",
        .parameters = {
            {
                .name = "mode",
                .descrption = "latest=最近一条，earliest=最早一条",
                .type = "string",
            },
        },
        .callback = fetch_memo,
    });
    registerTool(Tool{
        .name = "delete_memo",
        .descrption = "删除一条备忘录",
        .parameters = {
            {
                .name = "mode",
                .descrption = "latest=最近一条，earliest=最早一条",
                .type = "string",
            },
        },
        .callback = fetch_memo,
    });
    registerTool(Tool{
        .name = "fetch_all_memo",
        .descrption = "读取所有备忘录",
        .parameters = {
        },
        .callback = fetch_all_memo,
    });
    registerTool(Tool{
        .name = "delete_all_memo",
        .descrption = "删除所有备忘录（需用户确认）",
        .parameters = {
        },
        .callback = delete_all_memo,
    });
}
