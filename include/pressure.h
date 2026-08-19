#pragma once
#include <Arduino.h>

static constexpr uint8_t PRESSURE_SENSOR_ADDR = 0x76;

struct PressureData {
  float temperature; // °C
  float pressure;    // hPa
};

// Inicializa el bus I2C y el sensor. Llamar en setup().
// Devuelve true si el sensor responde correctamente.
bool pressureInit();

// Lee temperatura y presión.
// Devuelve true si la lectura es válida.
bool pressureRead(PressureData& data);

// Imprime los datos por Serial.
void pressurePrint(const PressureData& data);