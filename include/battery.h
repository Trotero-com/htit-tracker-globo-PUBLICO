#pragma once
#include <Arduino.h>

// Lee el ADC de batería habilitando el divisor solo durante la medida.
// Devuelve valor raw 12-bit (0..4095).
uint16_t readBatteryRaw();
