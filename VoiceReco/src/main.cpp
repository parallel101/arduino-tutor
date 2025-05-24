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
static const int MIN_POSITIVE_COUNT = 3 * 1024;
static const int MAX_PRELOGUE_COUNT = 6 * 1024;

static void recognize_voice()
{
    Serial.printf("Recognizing %zd bytes...\n", voiceGetAudioSize());
    String prompt = cloudQuery(voiceGetAudioBuffer(), voiceGetAudioSize());
    voiceClearAudioBuffer();
    if (prompt.isEmpty()) {
        Serial.printf("No prompt detected.\n");
        return;
    }
    Serial.printf("Prompt [%s]\n", prompt.c_str());

    String answer = aiChat(prompt);
    if (answer.isEmpty()) {
        Serial.printf("No answer generated.\n");
        return;
    }
    Serial.printf("Answer [%s]\n", answer.c_str());

    String options = "";
    size_t audio_bytes = cloudSynth(voiceGetAudioBuffer(), voiceGetAudioMaxSize(), answer, options);
    Serial.printf("Synthesized %d bytes.\n", audio_bytes);

    if (audio_bytes > 0) {
        voiceSetAudioSize(audio_bytes);
        Serial.printf("Volume tuned.\n");
        size_t bytes_played = voicePlayAudio();
        Serial.printf("Played %d bytes.\n", bytes_played);
        delay(300);
    }
    voiceClearAudioBuffer();
}

void loop()
{
    size_t bytes_read = voiceReadChunk();
    float rms_db = voiceRMSdB();
    Serial.printf("%f dB\n", rms_db);
    if (rms_db <= (positiveCount > 0 ? THRESHOLD_DB : START_THRESHOLD_DB)) {
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
        negativeCount = 0;
        positiveCount += bytes_read;
    }
}
