#pragma once

#include <Arduino.h>

void voiceSetup();
size_t voiceReadChunk();
float voiceRMSdB();
bool voiceBufferFull();
uint8_t *voiceGetAudioBuffer();
size_t voiceGetAudioSize();
void voiceSetAudioSize(size_t size);
size_t voiceGetAudioMaxSize();
void voiceClearAudioBuffer(size_t min_size = 0);
void voiceDecVolume(uint8_t dec);
size_t voicePlayAudio();
