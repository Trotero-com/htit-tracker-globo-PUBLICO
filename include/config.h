#pragma once
// Primero librerías externas
#include <Arduino.h>
// Luego ficheros propios del proyecto
#include "credentials.h"


// =====================
// GNSS (UC6580, UART1, solo RX)
// =====================
static constexpr int      PIN_VEXT_CTRL    = 3;
static constexpr int      PIN_GNSS_RST     = 35;
static constexpr int      PIN_GNSS_RX_ONLY = 33;
static constexpr uint32_t GNSS_BAUD        = 115200;

// Alias genéricos
static constexpr int PIN_VEXT    = PIN_VEXT_CTRL;
static constexpr int PIN_GNSS_RX = PIN_GNSS_RX_ONLY;

// =====================
// LoRa SX1262 (Wireless Tracker)
// =====================
static constexpr int PIN_LORA_NSS  = 8;
static constexpr int PIN_LORA_SCK  = 9;
static constexpr int PIN_LORA_MOSI = 10;
static constexpr int PIN_LORA_MISO = 11;
static constexpr int PIN_LORA_RST  = 12;
static constexpr int PIN_LORA_BUSY = 13;
static constexpr int PIN_LORA_DIO1 = 14;



// =====================
// Batería (Wireless Tracker — confirmado)
// =====================
static constexpr int PIN_BATT_ADC  = 1;  // VBAT_READ
static constexpr int PIN_BATT_CTRL = 2;  // ADC_CTRL (habilita divisor)

// =====================
// MAX31865 — PT100 2 hilos (SPI externo)
// =====================
static constexpr int PIN_MAX31865_CS   = 4;
static constexpr int PIN_MAX31865_SCK  = 5;
static constexpr int PIN_MAX31865_MISO = 6;
static constexpr int PIN_MAX31865_MOSI = 7;

// =====================
// Comportamiento
// =====================

#define SAMPLE_INTERVAL_MS  3000UL   // captura cada 3s
// Intervalo entre uplinks (ms) —  30000 hardcodeado en loop()
#define UPLINK_INTERVAL_MS 30000UL
#define JOIN_RETRY_MS  60000UL   // reintento join si no activado

// =====================
// Debug / Recovery
// =====================
// Poner a 1 SOLO para recuperar desincronización con TTN.
// Borra sesión+nonces y fuerza OTAA limpio. Volver a 0 después.
#define FORCE_CLEAN_JOIN 0

// =====================
// Session validation
// =====================
// Enviar 1 uplink CONFIRMED tras restore para validar sesión.
// Si no hay ACK se fuerza OTAA limpio automáticamente.
#define VALIDATE_SESSION_AFTER_RESTORE 0
#define SESSION_VALIDATE_FPORT         2