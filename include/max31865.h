#pragma once
#include <Arduino.h>

// Inicializa el MAX31865 PT100 3 hilos por SPI software.
void max31865Init();

// Lee temperatura del PT100 en °C.
// Devuelve -999.0f si hay fallo en la lectura.
float max31865ReadCelsius();

// Diagnóstico: imprime RTD raw, resistencia y código de fallo.
void max31865Debug();