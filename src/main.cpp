#include <Arduino.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioFileSourceBuffer.h>
#include <AudioOutput.h>
#include <driver/i2s.h>
#include "oh_no_mono.h"

#define I2S_WS   15
#define I2S_BCK  16
#define I2S_DATA 17

class RawI2SOutput : public AudioOutput {
public:
    RawI2SOutput() {
        i2s_config_t cfg = {
            .mode              = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate       = 44100,
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
            .data_in_num  = I2S_PIN_NO_CHANGE,
        };
        i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
        i2s_set_pin(I2S_NUM_0, &pins);
    }
    bool begin() override { return true; }
    bool SetRate(int hz) override { i2s_set_sample_rates(I2S_NUM_0, hz); return true; }
    bool ConsumeSample(int16_t sample[2]) override {
        int16_t buf[2] = { sample[LEFTCHANNEL], sample[RIGHTCHANNEL] };
        size_t written;
        i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
        return written == sizeof(buf);
    }
    bool stop() override { i2s_zero_dma_buffer(I2S_NUM_0); return true; }
};

static void mp3StatusCB(void*, int code, const char* str) {
    Serial.printf("[MP3 STATUS] code=%d  %s\n", code, str ? str : "");
}

AudioGeneratorMP3      *mp3;
AudioFileSourcePROGMEM *progmem;
AudioFileSourceBuffer  *file;
RawI2SOutput           *out;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.printf("Heap iniziale: %d byte\n", ESP.getFreeHeap());

    // Verifica lettura PROGMEM
    progmem = new AudioFileSourcePROGMEM(oh_no_mono, oh_no_mono_len);
    uint8_t hdr[8];
    progmem->read(hdr, 8);
    Serial.printf("PROGMEM header: ");
    for (int i = 0; i < 8; i++) Serial.printf("%02X ", hdr[i]);
    Serial.println();
    progmem->seek(0, SEEK_SET);

    // Buffer RAM tra PROGMEM e decoder (aiuta con la lettura a blocchi)
    file = new AudioFileSourceBuffer(progmem, 4096);

    out = new RawI2SOutput();
    Serial.printf("Heap dopo I2S: %d byte\n", ESP.getFreeHeap());

    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(mp3StatusCB, nullptr);
    Serial.printf("Heap prima di begin(): %d byte\n", ESP.getFreeHeap());

    bool ok = mp3->begin(file, out);
    Serial.printf("begin() = %s  |  Heap: %d byte\n", ok ? "OK" : "FAIL", ESP.getFreeHeap());

    if (!ok) {
        Serial.println("[ERRORE] Decoder non avviato.");
    } else {
        Serial.println("[OK] Riproduzione avviata...");
    }
}

void loop() {
    if (mp3 && mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            Serial.println("[OK] Riproduzione completata.");
        }
    }
}
