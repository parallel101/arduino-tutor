#include <Arduino.h>
#include <esp_system.h>
#include <driver/i2s.h>
#include <cstring>
#include <limits>

#if AUDIO_16KHZ
#define SAMPLE_RATE 16000
#else
#define SAMPLE_RATE 8000
#endif
#define SAMPLE_CHUNK_SIZE 1024
// #define SAMPLE_BUFFER_SIZE (1024 * SAMPLE_CHUNK_SIZE)
#define SAMPLE_BUFFER_SIZE (32 * SAMPLE_CHUNK_SIZE)
#define I2S_SCK GPIO_NUM_6
#define I2S_WS GPIO_NUM_7
#define I2S_DIN GPIO_NUM_3
#define I2S_DOUT GPIO_NUM_5
#define I2S_ENOUT GPIO_NUM_8

#if AUDIO_16BIT
typedef int16_t audio_sample_t;
#else
typedef int8_t audio_sample_t;
#endif

// don't mess around with this
static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
#if AUDIO_16BIT
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
#else
    .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,
#endif
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0,
};

// and don't mess around with this
static const i2s_pin_config_t i2s_pins = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DIN,
};

static audio_sample_t *sample_buffer;
static size_t current_sample = 0;
static float rms_db = -99.0f;

void voiceSetup()
{
    // pinMode(I2S_SPEAKER_ENABLE, OUTPUT);
    // digitalWrite(I2S_SPEAKER_ENABLE, LOW);

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2s_pins));

#if BOARD_HAS_PSRAM && 0
    if (!psramFound()) {
        printf("PSRAM not found!\n");
        delay(5000);
        abort();
    }
    psramInit();
    sample_buffer = (audio_sample_t *)ps_malloc(SAMPLE_BUFFER_SIZE * sizeof(audio_sample_t));
#else
    sample_buffer = (audio_sample_t *)heap_caps_malloc(SAMPLE_BUFFER_SIZE * sizeof(audio_sample_t), MALLOC_CAP_32BIT | MALLOC_CAP_DMA);
#endif
    if (!sample_buffer) {
        printf("Failed to allocate sample buffer!\n");
        delay(5000);
        abort();
    }
}

float voiceRMSdB()
{
    return rms_db;
}

uint8_t *voiceGetAudioBuffer()
{
    return (uint8_t *)sample_buffer;
}

size_t voiceGetAudioSize()
{
    return current_sample * sizeof(audio_sample_t);
}

void voiceSetAudioSize(size_t size)
{
    current_sample = size / sizeof(audio_sample_t);
    if (current_sample > SAMPLE_BUFFER_SIZE) {
        current_sample = SAMPLE_BUFFER_SIZE;
    }
}

size_t voiceGetAudioMaxSize()
{
    return SAMPLE_BUFFER_SIZE * sizeof(audio_sample_t);
}

void voiceClearAudioBuffer(size_t min_size)
{
    if (current_sample > min_size) {
        memmove(sample_buffer, sample_buffer + current_sample - min_size, min_size);
        current_sample = min_size;
    }
}

void voiceDecVolume(uint8_t dec)
{
    if (dec != 0) {
        for (size_t i = 0; i != current_sample; ++i) {
            sample_buffer[i] >>= dec;
        }
    }
}

size_t voicePlayAudio()
{
    digitalWrite(I2S_ENOUT, HIGH);
    size_t bytes_written = 0;
    ESP_ERROR_CHECK(i2s_write(I2S_NUM_0, sample_buffer, current_sample * sizeof(audio_sample_t), &bytes_written, portMAX_DELAY));
    digitalWrite(I2S_ENOUT, LOW);
    return bytes_written;
}

bool voiceBufferFull()
{
    return current_sample == SAMPLE_BUFFER_SIZE;
}

size_t voiceReadChunk()
{
    size_t bytes_read = 0;
    size_t readable_samples = std::min<size_t>(SAMPLE_CHUNK_SIZE, SAMPLE_BUFFER_SIZE - current_sample);
    if (readable_samples == 0) {
        rms_db = -99.0f;
        return 0;
    }
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, sample_buffer + current_sample, readable_samples * sizeof(audio_sample_t), &bytes_read, portMAX_DELAY));
    size_t samples_read = bytes_read / sizeof(audio_sample_t);
    size_t first_sample = current_sample;
    current_sample += samples_read;
    size_t last_sample = current_sample;

    float rms_samples = 0.0f;
    for (size_t i = first_sample; i != last_sample; ++i) {
        float sample = sample_buffer[i] * (1.0f / std::numeric_limits<audio_sample_t>::max());
        rms_samples += sq(sample);
    }
    rms_samples = sqrtf(rms_samples / samples_read);
    rms_db = 20.0f * log10f(rms_samples);
    // printf("%f dB\n", rms_db);
    // printf("%d\n", current_sample);
    return samples_read;
}
