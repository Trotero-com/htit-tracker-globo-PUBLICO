#include "payload.h"
#include "sampler.h"
#include <Arduino.h>
#include <math.h>   // roundf

// =========================================================
// Helpers de escritura big-endian
// =========================================================

static inline void writeU8(uint8_t* p, uint8_t v) {
    p[0] = v;
}

static inline void writeU16(uint8_t* p, uint16_t v) {
    p[0] = (v >> 8) & 0xFF;
    p[1] =  v       & 0xFF;
}

static inline void writeI16(uint8_t* p, int16_t v) {
    writeU16(p, (uint16_t)v);
}

// int24: solo los 24 bits bajos del int32
static inline void writeI24(uint8_t* p, int32_t v) {
    p[0] = (v >> 16) & 0xFF;
    p[1] = (v >>  8) & 0xFF;
    p[2] =  v        & 0xFF;
}

// =========================================================
// Conversión de unidades con clamp y redondeo correcto
// =========================================================

// Grados → int24 ×1e5  (rango ±83.88°, para España latitudes ~40°, lon ~3°)
static int32_t degToI24(float deg) {
    int32_t v = (int32_t)roundf(deg * 1e5f);
    if (v >  8388607) v =  8388607;   // clamp rango int24
    if (v < -8388608) v = -8388608;
    return v;
}

// m/s → uint16 cm/s
static uint16_t mpsToU16cms(float mps) {
    if (mps < 0.0f)    mps = 0.0f;
    float cms = mps * 100.0f;
    if (cms > 65535.0f) cms = 65535.0f;
    return (uint16_t)roundf(cms);
}

// metros → uint16
static uint16_t altToU16(float m) {
    if (m < 0.0f)     m = 0.0f;
    if (m > 65535.0f) m = 65535.0f;
    return (uint16_t)roundf(m);
}

// °C → int16 ×10  (0.1 °C)
static int16_t tempToI16d10(float c) {
    float v = roundf(c * 10.0f);
    if (v < -32768.0f) v = -32768.0f;
    if (v >  32767.0f) v =  32767.0f;
    return (int16_t)v;
}

// hPa → uint16 ×10  (0.1 hPa)
static uint16_t presToU16d10(float hpa) {
    float v = roundf(hpa * 10.0f);
    if (v < 0.0f)     v = 0.0f;
    if (v > 65535.0f) v = 65535.0f;
    return (uint16_t)v;
}

// =========================================================
// Codificación interna
// =========================================================

// Escribe una muestra COMPLETA (20 bytes) en p
static void encodeFull(uint8_t* p, const Sample* s, uint8_t tOffset) {
    uint8_t fixSats = s->sats & 0x7F;
    if (s->fix) fixSats |= 0x80;

    writeU8 (p +  0, tOffset);
    writeU8 (p +  1, fixSats);
    writeI24(p +  2, degToI24(s->lat));
    writeI24(p +  5, degToI24(s->lon));
    writeU16(p +  8, altToU16(s->alt_m));
    writeU16(p + 10, mpsToU16cms(s->speed_mps));
    writeI16(p + 12, tempToI16d10(s->temp_ext));
    writeI16(p + 14, tempToI16d10(s->temp_int));
    writeU16(p + 16, presToU16d10(s->pres_hpa));
    writeU16(p + 18, s->batt_raw);
}

// Escribe una MINI-muestra (11 bytes) en p
static void encodeMini(uint8_t* p, const Sample* s, uint8_t tOffset) {
    writeU8 (p + 0, tOffset);
    writeI24(p + 1, degToI24(s->lat));
    writeI24(p + 4, degToI24(s->lon));
    writeU16(p + 7, altToU16(s->alt_m));
    writeU16(p + 9, mpsToU16cms(s->speed_mps));
}

// =========================================================
// buildPacket — función pública
// =========================================================
size_t buildPacket(uint8_t* buf, uint8_t maxPayload, uint8_t* sentMinis) {
    *sentMinis = 0;

    uint8_t n = samplerCount();
    if (n == 0) {
        Serial.println("[PKT] Buffer vacío — sin muestras");
        return 0;
    }

    uint32_t uplinkTs = millis();

    // ---- Muestra completa: la más reciente del buffer ----
    const Sample* full = samplerGet(n - 1);
    if (!full) return 0;

    uint32_t diffFull  = uplinkTs - full->ts_ms;
    uint32_t secsFull  = diffFull / 1000U;
    uint8_t  tFull     = (secsFull > 255U) ? 255U : (uint8_t)secsFull;

    encodeFull(buf, full, tFull);
    size_t written = PKT_FULL_SIZE;

    // ---- Mini-muestras: las anteriores, las más antiguas primero ----
    if (maxPayload > PKT_FULL_SIZE) {
        uint8_t spaceLeft = maxPayload - (uint8_t)PKT_FULL_SIZE;
        uint8_t maxMinis  = spaceLeft / (uint8_t)PKT_MINI_SIZE;
        uint8_t available = (n > 1) ? (n - 1) : 0;
        uint8_t toSend    = (maxMinis < available) ? maxMinis : available;

        for (uint8_t i = 0; i < toSend; i++) {
            const Sample* mini = samplerGet(i);   // 0 = más antigua
            if (!mini) break;

            // Contar SIEMPRE para limpiar el buffer, aunque no se encode
            *sentMinis = i + 1;

            // Sin fix → no aporta posición útil, descartar del paquete
            if (!mini->fix) continue;

            uint32_t diffMini = uplinkTs - mini->ts_ms;
            uint32_t secsMini = diffMini / 1000U;
            uint8_t  tMini    = (secsMini > 255U) ? 255U : (uint8_t)secsMini;

            encodeMini(buf + written, mini, tMini);
            written += PKT_MINI_SIZE;
        }
    }

    Serial.print("[PKT] full=1  minis="); Serial.print(*sentMinis);
    Serial.print("  bytes=");             Serial.print(written);
    Serial.print("  maxDR=");             Serial.println(maxPayload);

    return written;
}

// =========================================================
// printPacket — impresión legible por Serial
// =========================================================
void printPacket(const uint8_t* buf, size_t len) {
    // Hex dump
    Serial.print("[PKT] hex=");
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        if (i + 1 < len) Serial.print(' ');
    }
    Serial.println();

    if (len < PKT_FULL_SIZE) return;

    // --- Decodificar muestra completa ---
    uint8_t tOff = buf[0];
    bool    fix  = (buf[1] & 0x80) != 0;
    uint8_t sats =  buf[1] & 0x7F;

    // Sign-extend int24 → int32
    int32_t latRaw = ((int32_t)buf[2] << 16) | ((int32_t)buf[3] << 8) | buf[4];
    if (latRaw & 0x800000) latRaw |= (int32_t)0xFF000000;

    int32_t lonRaw = ((int32_t)buf[5] << 16) | ((int32_t)buf[6] << 8) | buf[7];
    if (lonRaw & 0x800000) lonRaw |= (int32_t)0xFF000000;

    uint16_t alt  = ((uint16_t)buf[8]  << 8) | buf[9];
    uint16_t spd  = ((uint16_t)buf[10] << 8) | buf[11];
    int16_t  tExt = ((int16_t) buf[12] << 8) | buf[13];
    int16_t  tInt = ((int16_t) buf[14] << 8) | buf[15];
    uint16_t pres = ((uint16_t)buf[16] << 8) | buf[17];
    uint16_t batt = ((uint16_t)buf[18] << 8) | buf[19];

    Serial.print("[FULL] t-");    Serial.print(tOff);
    Serial.print("s fix=");       Serial.print(fix ? "SI" : "NO");
    Serial.print(" sats=");       Serial.print(sats);
    Serial.print(" lat=");        Serial.print(latRaw / 1e5f, 5);
    Serial.print(" lon=");        Serial.print(lonRaw / 1e5f, 5);
    Serial.print(" alt=");        Serial.print(alt);
    Serial.print("m spd=");       Serial.print(spd / 100.0f, 1);
    Serial.print("m/s t_ext=");   Serial.print(tExt / 10.0f, 1);
    Serial.print("C t_int=");     Serial.print(tInt / 10.0f, 1);
    Serial.print("C pres=");      Serial.print(pres / 10.0f, 1);
    Serial.print("hPa batt=");    Serial.println(batt);

    // --- Decodificar mini-muestras ---
    size_t  offset   = PKT_FULL_SIZE;
    uint8_t miniIdx  = 0;
    while (offset + PKT_MINI_SIZE <= len) {
        uint8_t mt = buf[offset];

        int32_t mlat = ((int32_t)buf[offset+1] << 16) | ((int32_t)buf[offset+2] << 8) | buf[offset+3];
        if (mlat & 0x800000) mlat |= (int32_t)0xFF000000;

        int32_t mlon = ((int32_t)buf[offset+4] << 16) | ((int32_t)buf[offset+5] << 8) | buf[offset+6];
        if (mlon & 0x800000) mlon |= (int32_t)0xFF000000;

        uint16_t malt = ((uint16_t)buf[offset+7] << 8) | buf[offset+8];
        uint16_t mspd = ((uint16_t)buf[offset+9] << 8) | buf[offset+10];

        Serial.print("[MINI"); Serial.print(miniIdx++);
        Serial.print("] t-");  Serial.print(mt);
        Serial.print("s lat="); Serial.print(mlat / 1e5f, 5);
        Serial.print(" lon=");  Serial.print(mlon / 1e5f, 5);
        Serial.print(" alt=");  Serial.print(malt);
        Serial.print("m spd="); Serial.print(mspd / 100.0f, 1);
        Serial.println("m/s");

        offset += PKT_MINI_SIZE;
    }
}
