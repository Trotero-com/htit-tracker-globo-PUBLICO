#pragma once
#include <Arduino.h>

// =========================================================
// Tamaños fijos del protocolo (bytes)
// =========================================================
#define PKT_FULL_SIZE  20   // muestra completa: todos los campos
#define PKT_MINI_SIZE  11   // mini-muestra: posición + speed
#define PKT_BUF_MAX   222   // máximo payload LoRaWAN EU868 DR5/4

// =========================================================
// Mapa de la muestra completa (20 bytes, big-endian)
//
// Byte  Campo          Tipo     Encoding
// ----  -------------  -------  ---------------------------
//  0    tiempo_offset  uint8    segundos antes del uplink
//  1    fix + sats     uint8    bit7=fix, bits0-6=sats
//  2-4  lat            int24    grados × 1e5
//  5-7  lon            int24    grados × 1e5
//  8-9  altitud        uint16   metros
// 10-11 speed_gps      uint16   cm/s  (m/s × 100)
// 12-13 temp_ext       int16    °C × 10  (PT100)
// 14-15 temp_int       int16    °C × 10  (BMP280)
// 16-17 presion        uint16   hPa × 10 (BMP280)
// 18-19 batt_raw       uint16   ADC 12-bit (0-4095)
//
// Mapa de la mini-muestra (11 bytes, big-endian)
//
// Byte  Campo          Tipo     Encoding
// ----  -------------  -------  ---------------------------
//  0    tiempo_offset  uint8    segundos antes del uplink
//  1-3  lat            int24    grados × 1e5
//  4-6  lon            int24    grados × 1e5
//  7-8  altitud        uint16   metros
//  9-10 speed_gps      uint16   cm/s
// =========================================================

// =========================================================
// Construye el paquete listo para enviar.
//
//   buf        → destino (debe tener >= PKT_BUF_MAX bytes)
//   maxPayload → node.getMaxPayloadLen()  — límite del DR actual
//   sentMinis  → OUTPUT: mini-muestras incluidas en el paquete
//                Usar para llamar samplerClear(1 + *sentMinis)
//                tras uplink exitoso.
//
// Devuelve bytes escritos en buf. 0 si el buffer del sampler
// está vacío.
// =========================================================
size_t buildPacket(uint8_t* buf, uint8_t maxPayload, uint8_t* sentMinis);

// Imprime el paquete en hex y en formato legible por Serial.
void printPacket(const uint8_t* buf, size_t len);