#pragma once
#include <Arduino.h>

// =========================================================
// Número de muestras en el buffer circular.
// 30 muestras × 3 s = 90 s de cobertura ante fallo de uplink.
// =========================================================
#define SAMPLER_BUF_SIZE 30

// =========================================================
// Muestra completa de todos los sensores.
// Se almacena en unidades flotantes — la conversión a bytes
// se hace en buildPacket(), no aquí.
// =========================================================
struct Sample {
    uint32_t ts_ms;      // millis() en el momento de captura
    bool     fix;        // true si GPS tiene posición válida
    uint8_t  sats;       // satélites con fix (0–20)
    float    lat;        // grados decimales
    float    lon;        // grados decimales
    float    alt_m;      // metros sobre nivel del mar
    float    speed_mps;  // velocidad GPS en m/s
    float    temp_ext;   // °C — PT100 exterior
    float    temp_int;   // °C — BMP280 interior
    float    pres_hpa;   // hPa — BMP280
    uint16_t batt_raw;   // ADC 12-bit (0–4095)
};

// =========================================================
// API pública
// =========================================================

// Inicializa buffer y temporizador. Llamar al final de setup().
void samplerInit();

// Comprobar si toca capturar. Llamar en CADA iteración del loop().
// Si han pasado SAMPLE_INTERVAL_MS desde la última captura,
// lee todos los sensores y almacena la muestra en el buffer.
void samplerTick();

// Número de muestras disponibles en el buffer (0 – SAMPLER_BUF_SIZE).
uint8_t samplerCount();

// Acceso por índice: 0 = más antigua, samplerCount()-1 = más reciente.
// Devuelve nullptr si idx >= samplerCount().
const Sample* samplerGet(uint8_t idx);

// Elimina las n muestras MÁS ANTIGUAS del buffer.
// Llamar tras un uplink exitoso para liberar las muestras enviadas.
void samplerClear(uint8_t n);
