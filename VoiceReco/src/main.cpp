#include <Arduino.h>
#include "WebPage.h"
#include "GlobalHTTP.h"
#include "CloudSTT.h"
#include "VoiceIO.h"
#include "AIChat.h"
#include "MemoRec.h"
#include "IRRemote.h"
#include "Thermometer.h"
#include "Assistant.h"
#include "secrets.h"

static int positiveCount = 0;
static int negativeCount = 0;

static const float START_THRESHOLD_DB = -58.0f;
static const float THRESHOLD_DB = -65.0f;
static const int MAX_NEGATIVE_COUNT = 7 * 1024;
static const int MIN_POSITIVE_COUNT = 5 * 1024;
static const int MAX_PRELOGUE_COUNT = 7 * 1024;

static void voice_play_callback(size_t audio_bytes)
{
    if (audio_bytes > 0) {
        printf("Playing %zu bytes...\n", audio_bytes);
        voiceSetAudioSize(audio_bytes);
        voiceDecVolume(1);
        size_t bytes_played = voicePlayAudio();
        printf("Played %zu bytes.\n", bytes_played);
    }
}

static void try_fix_prompt(String &prompt)
{
    if (prompt == "嗯。") {
        prompt.clear();
    } else if (prompt == "啊？") {
        prompt.clear();
    } else if (prompt == "嗯嗯。") {
        prompt.clear();
    }
}

static void try_fix_answer(String &answer)
{
    if (answer.startsWith("（")) {
        int index = answer.indexOf("）");
        if (index != -1) {
            answer = answer.substring(index + String("）").length());
        }
    }
}

static void play_string(String const &text)
{
    size_t audio_bytes = cloudSynth(
        voiceGetAudioBuffer(), voiceGetAudioMaxSize(),
        voice_play_callback, text, getSynthOptions());
    printf("Synthesized %zu bytes.\n", audio_bytes);
    voiceClearAudioBuffer();
}

static void recognize_voice()
{
    neopixelWrite(RGB_BUILTIN, 0, 0, 50);
    printf("Recognizing %zu bytes...\n", voiceGetAudioSize());
    String prompt = cloudQuery(voiceGetAudioBuffer(), voiceGetAudioSize());
    voiceClearAudioBuffer();
    try_fix_prompt(prompt);
    if (prompt.isEmpty()) {
        printf("No prompt detected.\n");
        digitalWrite(LED_BUILTIN, HIGH);
        return;
    }
    printf("Prompt [%s]\n", prompt.c_str());
    resetAssistantSleep();

    neopixelWrite(RGB_BUILTIN, 0, 50, 50);
    // aiChatReset();
    String answer = aiChat(prompt);
    try_fix_answer(answer);
    if (answer.isEmpty()) {
        printf("No answer generated.\n");
        return;
    }
    printf("Answer [%s]\n", answer.c_str());
    neopixelWrite(RGB_BUILTIN, 0, 0, 50);
    play_string(answer);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    delay(100);
}

void setup()
{
    pinMode(RGB_BUILTIN, OUTPUT);

    neopixelWrite(RGB_BUILTIN, 20, 20, 20);
    printf("webSetup...\n");
    webSetup();
    printf("httpSetup...\n");
    httpSetup();
    printf("cloudSetup...\n");
    cloudSetup();
    printf("aiChatSetup...\n");
    aiChatSetup();
    printf("voiceSetup...\n");
    voiceSetup();
    printf("thermoSetup...\n");
    thermoSetup();
    printf("remoteSetup...\n");
    remoteSetup();
    printf("memoSetup...\n");
    memoSetup();
    printf("assistantSetup...\n");
    assistantSetup();
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    printf("setup complete.\n");

    // play_string("WiFi 已连接，地址：" + WiFi.localIP().toString());
}


void loop()
{
    int brightness = aiChatGetLevel() >= 1 ? 40 : 1;
    size_t bytes_read = voiceReadChunk();
    float rms_db = voiceRMSdB();
    // printf("%f dB\n", rms_db);
    if (voiceBufferFull()) {
        digitalWrite(LED_BUILTIN, HIGH);
        printf("Voice too long.\n");
        voiceClearAudioBuffer();
        negativeCount = 0;
        positiveCount = 0;
    } else if (rms_db <= (positiveCount > 0 ? THRESHOLD_DB : START_THRESHOLD_DB)) {
        neopixelWrite(RGB_BUILTIN, brightness, 0, 0);
        if (positiveCount > 0) {
            negativeCount += bytes_read;
            if (negativeCount >= MAX_NEGATIVE_COUNT) {
                if (positiveCount >= MIN_POSITIVE_COUNT) {
                    recognize_voice();
                } else {
                    printf("Voice too short.\n");
                }
                negativeCount = 0;
                positiveCount = 0;
            }
        } else {
            voiceClearAudioBuffer(MAX_PRELOGUE_COUNT);
            updateAssistantSleep();
        }
    } else {
        neopixelWrite(RGB_BUILTIN, 0, brightness, 0);
        negativeCount = 0;
        positiveCount += bytes_read;
    }
}
