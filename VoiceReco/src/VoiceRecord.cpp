#include <Arduino.h>
#include <esp_system.h>
#include <driver/i2s.h>

#define SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE (SAMPLE_RATE * 5)
#define SAMPLE_CHUNK_SIZE 1024
#define I2S_MIC_SERIAL_CLOCK 3
#define I2S_MIC_LEFT_RIGHT_CLOCK 2
#define I2S_MIC_SERIAL_DATA 4

// don't mess around with this
static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
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
static const i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA,
};

static int16_t *sample_buffer;
static size_t current_sample = 0;
static float rms_db = -99.0f;

void voiceSetup()
{
    // start up the I2S peripheral
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2s_mic_pins));

    sample_buffer = (int16_t *)heap_caps_malloc(SAMPLE_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_32BIT | MALLOC_CAP_DMA);
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
    return current_sample * sizeof(int16_t);
}

void voiceClearAudioBuffer()
{
    current_sample = 0;
}

void voiceLoop()
{
    size_t bytes_read = 0;
    size_t readable_samples = std::min<size_t>(SAMPLE_CHUNK_SIZE, SAMPLE_BUFFER_SIZE - current_sample);
    if (readable_samples == 0) {
        rms_db = -99.0f;
        return;
    }
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, sample_buffer + current_sample, sizeof(int16_t) * readable_samples, &bytes_read, portMAX_DELAY));
    size_t samples_read = bytes_read / sizeof(int16_t);
    size_t first_sample = current_sample;
    current_sample += samples_read;
    size_t last_sample = current_sample;

    float rms_samples = 0.0f;
    for (size_t i = first_sample; i != last_sample; ++i) {
        float sample = sample_buffer[i] * (1.0f / INT16_MAX);
        rms_samples += sq(sample);
    }
    rms_samples = sqrtf(rms_samples / samples_read);
    rms_db = 20.0f * log10f(rms_samples);
    // Serial.printf("%f dB\n", rms_db);
    // Serial.printf("%d\n", current_sample);
}
