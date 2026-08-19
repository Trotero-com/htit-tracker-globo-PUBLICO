#pragma once
#include <Arduino.h>

// Convierte array EUI big-endian (8 bytes) a uint64_t
static inline uint64_t euiToU64(const uint8_t eui[8]) {
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) v = (v << 8) | eui[i];
  return v;
}

// Imprime N bytes en hexadecimal por Serial
static inline void printHex(const uint8_t* b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (b[i] < 0x10) Serial.print('0');
    Serial.print(b[i], HEX);
  }
}
