#include "pressure.h"
#include <Wire.h>
#include <MS5611.h>

static MS5611 sensor(PRESSURE_SENSOR_ADDR);
static bool initialized = false;

bool pressureInit() {
  Wire.begin(17, 18); // SDA=17, SCL=18
  Wire.setTimeout(1000);

  if (!sensor.begin()) {
    Serial.println("[MS5611] Sensor no encontrado en 0x76 ❌");
    initialized = false;
    return false;
  }

  sensor.setOversampling(OSR_ULTRA_HIGH);
  initialized = true;
  Serial.println("[MS5611] Init OK ✅");
  return true;
}

bool pressureRead(PressureData& data) {
  if (!initialized) return false;

  int result = sensor.read();
  if (result != MS5611_READ_OK) {
    Serial.print("[MS5611] Error de lectura: ");
    Serial.println(result);
    return false;
  }

  data.temperature = sensor.getTemperature();
  data.pressure    = sensor.getPressure();
  return true;
}

void pressurePrint(const PressureData& data) {
  Serial.print("[MS5611] temp="); Serial.print(data.temperature, 1); Serial.print(" °C");
  Serial.print("  pres=");        Serial.print(data.pressure, 1);    Serial.println(" hPa");
}