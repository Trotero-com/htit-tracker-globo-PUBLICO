#include "max31865.h"
#include "config.h"
#include <SPI.h>
#include <Adafruit_MAX31865.h>

// SPI hardware con pines personalizados
static SPIClass spi2(HSPI);
static Adafruit_MAX31865 thermo(PIN_MAX31865_CS, &spi2);

 static const float RREF     = 430.0f;
//static const float RREF     = 438.0f;
static const float RNOMINAL = 100.0f;
static bool initialized = false;

void max31865Init() {
  spi2.begin(PIN_MAX31865_SCK,
             PIN_MAX31865_MISO,
             PIN_MAX31865_MOSI,
             PIN_MAX31865_CS);
//  thermo.begin(MAX31865_3WIRE);
  thermo.begin(MAX31865_2WIRE);   // cambiar de 3WIRE a 2WIRE
  initialized = true;
  Serial.println("[PT100] MAX31865 Init OK ✅");
}

float max31865ReadCelsius() {
  if (!initialized) return -999.0f;
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("[PT100] Fault: 0x"); Serial.println(fault, HEX);
    thermo.clearFault();
    return -999.0f;
  }
  return thermo.temperature(RNOMINAL, RREF);
}

void max31865Debug() {
  uint16_t rtd  = thermo.readRTD();
  uint8_t fault = thermo.readFault();
  Serial.print("[PT100] RTD raw: ");        Serial.println(rtd);
  Serial.print("[PT100] Resistencia: ");
  Serial.print((rtd / 32768.0f) * RREF);   Serial.println(" Ω");
  Serial.print("[PT100] Fault: 0x");        Serial.println(fault, HEX);
}