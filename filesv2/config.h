#pragma once
#include <Arduino.h>

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
// TTN OTAA (EU868)
// =====================
// >>> CLAVES ELIMINADAS ANTES DE PUBLICAR <<<
//
// Esta versión antigua llevaba las credenciales escritas directamente aquí.
// Se han sustituido por ceros. En la versión actual del firmware las claves
// viven en include/credentials.h (fuera del control de versiones).
//
// Ver include/credentials.h.example para la plantilla y las instrucciones.
static const uint8_t JOIN_EUI[8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t DEV_EUI[8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // <-- ELIMINADO
};
static const uint8_t APP_KEY[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // <-- ELIMINADO
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t NWK_KEY[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // <-- ELIMINADO
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// =====================
// Batería (Wireless Tracker — confirmado)
// =====================
static constexpr int PIN_BATT_ADC  = 1;  // VBAT_READ
static constexpr int PIN_BATT_CTRL = 2;  // ADC_CTRL (habilita divisor)

// =====================
// Comportamiento
// =====================

// Intervalo entre uplinks (ms) — era 60000 hardcodeado en loop()
#define UPLINK_INTERVAL_MS 60000UL

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
#define VALIDATE_SESSION_AFTER_RESTORE 1
#define SESSION_VALIDATE_FPORT         2
