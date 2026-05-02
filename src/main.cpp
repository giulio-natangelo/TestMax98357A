#include <Arduino.h>
#include <SPIFFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioOutput.h>
#include <driver/i2s.h>

// Pin MAX98357A → ESP32-S3
#define I2S_WS   15  // LRC / Word Select
#define I2S_BCK  16  // Bit Clock
#define I2S_DATA 17  // DIN / Data

// Output personalizzato: usa il driver I2S raw (che funziona)
// invece dell'inizializzazione interna di AudioOutputI2S
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

    // Chiamato dal decoder quando cambia il sample rate dell'MP3
    bool SetRate(int hz) override {
        i2s_set_sample_rates(I2S_NUM_0, hz);
        return true;
    }

    // Chiamato dal decoder per ogni campione stereo decodificato
    bool ConsumeSample(int16_t sample[2]) override {
        int16_t buf[2] = { sample[LEFTCHANNEL], sample[RIGHTCHANNEL] };
        size_t written;
        i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
        return written == sizeof(buf);
    }

    bool stop() override {
        i2s_zero_dma_buffer(I2S_NUM_0);
        return true;
    }
};

AudioGeneratorMP3     *mp3;
AudioFileSourceSPIFFS *file;
RawI2SOutput          *out;

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!SPIFFS.begin(true)) {
        Serial.println("[ERRORE] Mount SPIFFS fallito.");
        return;
    }
    Serial.println("[OK] SPIFFS montato.");

    // Lista file nel filesystem
    Serial.println("--- File su SPIFFS ---");
    File root = SPIFFS.open("/");
    File f = root.openNextFile();
    while (f) {
        Serial.printf("  %s  (%d byte)\n", f.name(), f.size());
        f = root.openNextFile();
    }
    Serial.println("----------------------");

    Serial.printf("Heap libero: %d byte\n", ESP.getFreeHeap());

    file = new AudioFileSourceSPIFFS("/oh-no-mono.mp3");
    if (!file->isOpen()) {
        Serial.println("[ERRORE] File /oh-no-mono.mp3 non trovato.");
        return;
    }
    Serial.printf("[OK] File aperto, dimensione: %d byte\n", (int)file->getSize());

    // Leggi i primi 4 byte per verificare l'header MP3
    uint8_t header[4];
    file->read(header, 4);
    Serial.printf("[DEBUG] Primi 4 byte: %02X %02X %02X %02X\n",
        header[0], header[1], header[2], header[3]);
    file->seek(0, SEEK_SET);  // torna all'inizio

    out = new RawI2SOutput();
    Serial.printf("Heap dopo I2S init: %d byte\n", ESP.getFreeHeap());

    mp3 = new AudioGeneratorMP3();
    if (!mp3->begin(file, out)) {
        Serial.println("[ERRORE] Impossibile avviare il decoder MP3.");
        return;
    }
    Serial.println("[OK] Riproduzione avviata...");
}

void loop() {
    if (mp3 && mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            Serial.println("[OK] Riproduzione completata.");
        }
    }
}
