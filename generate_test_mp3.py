"""
Genera data/test.mp3 — tono sinusoidale mono 440 Hz, 3 secondi, 44100 Hz, 128 kbps.
Richiede ffmpeg o lame installato sul sistema.

Uso:
    python3 generate_test_mp3.py
    (oppure doppio click su Windows se Python è nel PATH)

Dopo aver eseguito questo script, la cartella data/ conterrà test.mp3.
Poi in PlatformIO: "Upload Filesystem Image" → "Upload".
"""

import math
import wave
import struct
import subprocess
import os
import sys

# ── Parametri audio ──────────────────────────────────────────────────────────
SAMPLE_RATE = 44100
DURATION    = 3        # secondi
FREQUENCY   = 440      # Hz  (La4 / A4)
AMPLITUDE   = 0.5      # 0.0 – 1.0

DATA_DIR    = os.path.join(os.path.dirname(__file__), "data")
WAV_PATH    = os.path.join(DATA_DIR, "test.wav")
MP3_PATH    = os.path.join(DATA_DIR, "test.mp3")

# ── 1. Crea la cartella data/ se non esiste ───────────────────────────────────
os.makedirs(DATA_DIR, exist_ok=True)

# ── 2. Genera il file WAV con onda sinusoidale ────────────────────────────────
print(f"[1/3] Generazione onda sinusoidale {FREQUENCY} Hz, {DURATION}s, mono...")
num_samples = int(SAMPLE_RATE * DURATION)
samples = [
    int(AMPLITUDE * math.sin(2 * math.pi * FREQUENCY * i / SAMPLE_RATE) * 32767)
    for i in range(num_samples)
]

with wave.open(WAV_PATH, "w") as f:
    f.setnchannels(1)          # mono
    f.setsampwidth(2)          # 16-bit
    f.setframerate(SAMPLE_RATE)
    packed = struct.pack(f"<{num_samples}h", *samples)
    f.writeframes(packed)

print(f"    WAV temporaneo: {WAV_PATH}")

# ── 3. Converti WAV → MP3 (ffmpeg oppure lame) ───────────────────────────────
print("[2/3] Conversione WAV → MP3...")

ENCODERS = [
    ["ffmpeg", "-y", "-i", WAV_PATH, "-ar", str(SAMPLE_RATE),
     "-ac", "1", "-b:a", "128k", MP3_PATH],
    ["lame", "--preset", "standard", "-m", "m", WAV_PATH, MP3_PATH],
]

converted = False
for cmd in ENCODERS:
    try:
        result = subprocess.run(cmd, capture_output=True, timeout=60)
        if result.returncode == 0:
            converted = True
            break
        else:
            # mostra stderr solo in caso di errore reale
            err = result.stderr.decode(errors="replace").strip()
            if err:
                print(f"    [{cmd[0]}] {err[-200:]}")
    except FileNotFoundError:
        print(f"    {cmd[0]} non trovato, provo il prossimo...")
    except subprocess.TimeoutExpired:
        print(f"    {cmd[0]} timeout.")

# ── 4. Risultato ─────────────────────────────────────────────────────────────
if converted:
    os.remove(WAV_PATH)
    size = os.path.getsize(MP3_PATH)
    print(f"[3/3] ✓ MP3 creato: {MP3_PATH}  ({size} byte)")
    print()
    print("Prossimi passi in PlatformIO:")
    print("  1. Esegui 'Upload Filesystem Image'  (carica data/test.mp3 su LittleFS)")
    print("  2. Esegui 'Upload'                   (carica il firmware)")
    print("  3. Apri il Monitor seriale → dovresti sentire il tono dal MAX98357A.")
else:
    print()
    print("⚠  ffmpeg e lame non trovati. Il file WAV è pronto ma non è stato convertito in MP3.")
    print()
    print("Opzione A – Installa ffmpeg:")
    print("  Windows: https://www.gyan.dev/ffmpeg/builds/  (aggiungi bin/ al PATH)")
    print("  Poi rilancia questo script.")
    print()
    print("Opzione B – Usa il WAV al posto dell'MP3:")
    print("  Rinomina data/test.wav → data/test.mp3  (NON funzionerà correttamente)")
    print("  Oppure modifica main.cpp sostituendo:")
    print("    AudioGeneratorMP3  → AudioGeneratorWAV")
    print("    AudioFileSourceLittleFS('/test.mp3')  → AudioFileSourceLittleFS('/test.wav')")
    print("  e aggiungendo:  #include <AudioGeneratorWAV.h>")
    sys.exit(1)
