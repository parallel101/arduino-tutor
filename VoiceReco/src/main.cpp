#include <Arduino.h>
#include "NetConnect.h"
#include "CloudSTT.h"
#include "VoiceIO.h"
#include "AIChat.h"

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    netSetup();
    cloudSetup();
    voiceSetup();
}

static int positiveCount = 0;
static int negativeCount = 0;

static const float START_THRESHOLD_DB = -53.0f;
static const float THRESHOLD_DB = -61.0f;
static const int MAX_NEGATIVE_COUNT = 7 * 1024;
static const int MIN_POSITIVE_COUNT = 4 * 1024;
static const int MAX_PRELOGUE_COUNT = 6 * 1024;

static void voice_play_callback(size_t audio_bytes)
{
    if (audio_bytes > 0) {
        Serial.printf("Playing %d bytes...\n", audio_bytes);
        voiceSetAudioSize(audio_bytes);
        size_t bytes_played = voicePlayAudio();
        Serial.printf("Played %d bytes.\n", bytes_played);
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

static void recognize_voice()
{
    digitalWrite(LED_BUILTIN, LOW);
    Serial.printf("Recognizing %zd bytes...\n", voiceGetAudioSize());
    String prompt = cloudQuery(voiceGetAudioBuffer(), voiceGetAudioSize());
    voiceClearAudioBuffer();
    try_fix_prompt(prompt);
    if (prompt.isEmpty()) {
        Serial.printf("No prompt detected.\n");
        digitalWrite(LED_BUILTIN, HIGH);
        return;
    }
    Serial.printf("Prompt [%s]\n", prompt.c_str());

    digitalWrite(LED_BUILTIN, HIGH);
    String answer = aiChat(prompt);
    try_fix_answer(answer);
    if (answer.isEmpty()) {
        Serial.printf("No answer generated.\n");
        return;
    }
    Serial.printf("Answer [%s]\n", answer.c_str());
    digitalWrite(LED_BUILTIN, LOW);

    String options = "";
    size_t audio_bytes = cloudSynth(
        voiceGetAudioBuffer(), voiceGetAudioMaxSize(),
        voice_play_callback, answer, options);
    Serial.printf("Synthesized %d bytes.\n", audio_bytes);

    voiceClearAudioBuffer();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
}

void loop()
{
    size_t bytes_read = voiceReadChunk();
    float rms_db = voiceRMSdB();
    Serial.printf("%f dB\n", rms_db);
    if (voiceBufferFull()) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.printf("Voice too long.\n");
        voiceClearAudioBuffer();
        negativeCount = 0;
        positiveCount = 0;
    } else if (rms_db <= (positiveCount > 0 ? THRESHOLD_DB : START_THRESHOLD_DB)) {
        digitalWrite(LED_BUILTIN, HIGH);
        if (positiveCount > 0) {
            negativeCount += bytes_read;
            if (negativeCount >= MAX_NEGATIVE_COUNT) {
                if (positiveCount >= MIN_POSITIVE_COUNT) {
                    recognize_voice();
                } else {
                    Serial.printf("Voice too short.\n");
                }
                negativeCount = 0;
                positiveCount = 0;
            }
        } else {
            voiceClearAudioBuffer(MAX_PRELOGUE_COUNT);
        }
    } else {
        digitalWrite(LED_BUILTIN, LOW);
        negativeCount = 0;
        positiveCount += bytes_read;
    }
}
