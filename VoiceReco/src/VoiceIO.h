#pragma once

#include <Arduino.h>

void voiceSetup();
bool voiceReadChunk();
float voiceRMSdB();
uint8_t *voiceGetAudioBuffer();
size_t voiceGetAudioSize();
void voiceSetAudioSize(size_t size);
size_t voiceGetAudioMaxSize();
void voiceClearAudioBuffer();
size_t voicePlayAudio();
