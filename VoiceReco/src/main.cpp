#include <Arduino.h>

void netSetup();
void cloudSetup();
String cloudQuery(uint8_t *audio, size_t size);
void voiceSetup();
void voiceLoop();
float voiceRMSdB();
uint8_t *voiceGetAudioBuffer();
size_t voiceGetAudioSize();
void voiceClearAudioBuffer();

void setup()
{
    Serial.begin(115200);
    delay(100);
    pinMode(LED_BUILTIN, OUTPUT);

    netSetup();
    cloudSetup();
    voiceSetup();
}

static int positiveCount = 0;
static int negativeCount = 0;

static const float THRESHOLD_DB = -52.0f;
static const int MAX_NEGATIVE_COUNT = 5;
static const int MIN_POSITIVE_COUNT = 3;

void loop()
{
    voiceLoop();
    float rms_db = voiceRMSdB();
    Serial.printf("%f dB\n", rms_db);
    if (rms_db <= THRESHOLD_DB) {
        if (positiveCount > 0) {
            ++negativeCount;
            if (negativeCount >= MAX_NEGATIVE_COUNT) {
                if (positiveCount >= MIN_POSITIVE_COUNT) {
                    Serial.printf("Recognizing %zd samples...\n", voiceGetAudioSize());
                    String result = cloudQuery(voiceGetAudioBuffer(), voiceGetAudioSize());
                    voiceClearAudioBuffer();
                    Serial.printf("Speech [%s]\n", result.c_str());
                } else {
                    Serial.printf("Voice too short\n");
                }
                negativeCount = 0;
                positiveCount = 0;
            }
        }
    } else {
        negativeCount = 0;
        ++positiveCount;
    }
}
