#include <Arduino.h>
#include <esp_system.h>
#include <driver/i2s.h>

#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_RATE 16000
#define I2S_MIC_SERIAL_CLOCK 9
#define I2S_MIC_LEFT_RIGHT_CLOCK 10
#define I2S_MIC_SERIAL_DATA 21

// don't mess around with this
i2s_config_t i2s_config = {
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
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA,
};

void voiceSetup()
{
    // start up the I2S peripheral
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2s_mic_pins));
}

int16_t int16_samples[SAMPLE_BUFFER_SIZE];

void voiceLoop()
{
    size_t bytes_read = 0;
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, int16_samples, sizeof(int16_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY));
    size_t samples_read = bytes_read / sizeof(int16_t);

    float rms_samples = 0.0f;
    for (size_t i = 0; i < samples_read; i++)
    {
        float sample = int16_samples[i] * (1.0f / INT16_MAX);
        rms_samples += sq(sample);
    }
    rms_samples = sqrtf(rms_samples);
    float rms_db = 20.0f * log10f(rms_samples);
    Serial.printf("%f dB\n", rms_db);
}
