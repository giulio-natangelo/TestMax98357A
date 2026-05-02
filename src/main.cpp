#include <Arduino.h>
#include <SPIFFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioOutputI2S.h>
#include <esp_partition.h>

// Pin MAX98357A → ESP32-S3
#define I2S_WS   5   // LRC / Word Select
#define I2S_BCK  6   // Bit Clock
#define I2S_DATA 7   // DIN / Data

AudioGeneratorMP3    *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S       *out;

void printPartitions() {
    Serial.println("--- Partizioni sul flash ---");
    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it) {
        const esp_partition_t* p = esp_partition_get(it);
        Serial.printf("  label=%-12s  subtype=%d  offset=0x%06x  size=%d KB\n",
            p->label, p->subtype, p->address, p->size / 1024);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    Serial.println("----------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(4500);

    printPartitions();

    if (!SPIFFS.begin(true)) {
        Serial.println("[ERRORE] Mount SPIFFS fallito.");
        return;
    }
    Serial.println("[OK] SPIFFS montato.");

    file = new AudioFileSourceSPIFFS("/oh-no-mono.mp3");
    if (!file->isOpen()) {
        Serial.println("[ERRORE] File /oh-no-mono.mp3 non trovato.");
        return;
    }

    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DATA);
    out->SetOutputModeMono(true);
    out->SetGain(0.5);

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
