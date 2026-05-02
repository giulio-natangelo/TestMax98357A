#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

// Cambia questi se vuoi testare altri pin
#define I2S_WS   15
#define I2S_BCK  16
#define I2S_DATA 17

#define SAMPLE_RATE 44100
#define FREQ        440        // Hz - tono La
#define AMPLITUDE   28000      // 0-32767, quasi al massimo

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("[TEST] Avvio tono I2S diretto a 440 Hz...");

    i2s_config_t cfg = {
        .mode              = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate       = SAMPLE_RATE,
        .bits_per_sample   = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format    = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags  = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count     = 8,
        .dma_buf_len       = 64,
        .use_apll          = false,
    };

    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCK,
        .ws_io_num    = I2S_WS,
        .data_out_num = I2S_DATA,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };

    esp_err_t err;
    err = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    Serial.printf("[TEST] i2s_driver_install: %s\n", esp_err_to_name(err));

    err = i2s_set_pin(I2S_NUM_0, &pins);
    Serial.printf("[TEST] i2s_set_pin: %s\n", esp_err_to_name(err));

    Serial.println("[TEST] Tono in uscita — dovresti sentire un La continuo.");
}

void loop() {
    static float phase = 0.0f;
    const int N = 64;
    int16_t buf[N * 2];  // stereo: L + R interlacciati

    for (int i = 0; i < N; i++) {
        int16_t val = (int16_t)(AMPLITUDE * sinf(phase));
        buf[i * 2]     = val;  // canale L
        buf[i * 2 + 1] = val;  // canale R
        phase += 2.0f * M_PI * FREQ / SAMPLE_RATE;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
    }

    size_t written;
    i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
}
