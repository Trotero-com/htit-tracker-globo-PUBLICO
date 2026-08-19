#include "battery.h"
#include "config.h"

uint16_t readBatteryRaw() {
  // Habilita divisor solo durante la medida
  pinMode(PIN_BATT_CTRL, OUTPUT);
  digitalWrite(PIN_BATT_CTRL, HIGH);
  delay(10);

  analogReadResolution(12); // 0..4095
  uint16_t raw = (uint16_t)analogRead(PIN_BATT_ADC);

  // Apaga divisor para ahorrar
  digitalWrite(PIN_BATT_CTRL, LOW);

  return raw;
}
