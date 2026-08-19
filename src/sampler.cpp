#include "sampler.h"
#include "gnss.h"
#include "pressure.h"
#include "max31865.h"
#include "battery.h"
#include "config.h"
#include <Arduino.h>

// =========================================================
// Buffer circular interno
// =========================================================
static Sample  buf[SAMPLER_BUF_SIZE];
static uint8_t head  = 0;   // índice de la muestra más antigua
static uint8_t count = 0;   // muestras válidas actualmente en buf
static uint32_t lastCapture = 0;

// =========================================================
// Captura interna — lee todos los sensores en el instante actual
// =========================================================
static void capture() {
    Sample s = {};
    s.ts_ms = millis();

    // --- GNSS ---
    s.fix  = gps.location.isValid();
    s.sats = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;

    if (s.fix) {
        s.lat = (float)gps.location.lat();
        s.lon = (float)gps.location.lng();
    }
    if (gps.altitude.isValid()) {
        s.alt_m = (float)gps.altitude.meters();
    }
    if (s.fix && gps.speed.isValid()) {
        float v = (float)gps.speed.mps();
        s.speed_mps = (v > 0.0f) ? v : 0.0f;
    }

    // --- MS5611 ---
    PressureData bme = {};
    if (pressureRead(bme)) {
        s.temp_int = bme.temperature;
        s.pres_hpa = bme.pressure;
    }

    // --- PT100 ---
    float te = max31865ReadCelsius();
    s.temp_ext = isnan(te) ? 0.0f : te;

    // --- Batería ---
    s.batt_raw = readBatteryRaw();

    // --- Push al buffer circular ---
    // Si el buffer está lleno, sobreescribe la muestra más antigua
    uint8_t tail = (head + count) % SAMPLER_BUF_SIZE;
    buf[tail] = s;
    if (count < SAMPLER_BUF_SIZE) {
        count++;
    } else {
        head = (head + 1) % SAMPLER_BUF_SIZE;  // descarta la más antigua
    }

    Serial.print("[SMPL] ts="); Serial.print(s.ts_ms / 1000);
    Serial.print("s  fix=");    Serial.print(s.fix);
    Serial.print("  sats=");    Serial.print(s.sats);
    Serial.print("  alt=");     Serial.print(s.alt_m, 0);
    Serial.print("m  buf=");    Serial.print(count);
    Serial.println();
}

// =========================================================
// API pública
// =========================================================

void samplerInit() {
    head        = 0;
    count       = 0;
    lastCapture = 0;   // primera captura en el primer tick
    Serial.print("[SMPL] Init — buf="); Serial.print(SAMPLER_BUF_SIZE);
    Serial.print("  interval=");       Serial.print(SAMPLE_INTERVAL_MS);
    Serial.println("ms");
}

void samplerTick() {
    uint32_t now = millis();
    if (now - lastCapture >= SAMPLE_INTERVAL_MS) {
        lastCapture = now;
        capture();
    }
}

uint8_t samplerCount() {
    return count;
}

const Sample* samplerGet(uint8_t idx) {
    if (idx >= count) return nullptr;
    return &buf[(head + idx) % SAMPLER_BUF_SIZE];
}

void samplerClear(uint8_t n) {
    if (n > count) n = count;
    head   = (uint8_t)((head + n) % SAMPLER_BUF_SIZE);
    count -= n;
}
