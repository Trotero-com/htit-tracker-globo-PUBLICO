#pragma once
#include <Arduino.h>

// Tamaño actual del payload (bytes)
// Byte  Campo
//  0    fix        (uint8)
//  1    sats       (uint8)
//  2-5  lat        (int32  ×1e7, BE)
//  6-9  lon        (int32  ×1e7, BE)
// 10-11 alt        (int16  m,    BE)
// 12-13 speed_cms  (uint16 cm/s, BE)
// 14-15 course_cdeg(uint16 cdeg, BE)
// 16-17 batt_raw   (uint16 ADC,  BE)
#define PAYLOAD_SIZE 18

// Rellena out[PAYLOAD_SIZE] con los datos actuales de GPS y batería.
void buildPayload(uint8_t out[PAYLOAD_SIZE]);

// Imprime el payload en hex y en formato legible por Serial.
void printPayload(const uint8_t* p);
