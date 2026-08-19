#include "lora.h"
#include "nvs.h"
#include "utils.h"
#include "config.h"
#include <Arduino.h>
#include <SPI.h>

// =============================================================================
// Objetos RadioLib — accesibles via extern desde nvs.cpp y main.cpp
// =============================================================================
SX1262     radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);
LoRaWANNode node(&radio, &EU868);

// =============================================================================

bool loraJoinOk(int16_t st) {
 // Serial.print("[DEBUG] loraJoinOk st="); Serial.print(st);
 // Serial.print(" NEW_SESSION="); Serial.print(RADIOLIB_LORAWAN_NEW_SESSION);
 // Serial.print(" RESTORED="); Serial.println(RADIOLIB_LORAWAN_SESSION_RESTORED);
  return (st == RADIOLIB_ERR_NONE)            ||
         (st == RADIOLIB_LORAWAN_NEW_SESSION)  ||
         (st == RADIOLIB_LORAWAN_SESSION_RESTORED);
}



bool loraInit() {
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  delay(50);

  Serial.println("BOOT: radio.begin...");
//  int16_t st = radio.begin(868.1);
int16_t st = radio.begin(
  868.1,                                    // frecuencia
  125.0,                                    // BW
  9,                                        // SF
  7,                                        // CR
  RADIOLIB_SX126X_SYNC_WORD_PRIVATE,        // syncWord
  10,                                       // power
  8,                                        // preamble
  1.8                                       // TCXO voltage ← cambio clave
);

  Serial.print("BOOT: radio.begin ret = "); Serial.println(st);
  if (st != RADIOLIB_ERR_NONE) return false;

  node.scanGuard = 50;  // ms extra de margen en ventana RX (default ~10ms)
  //radio.setOutputPower(22);  // máxima potencia TX (21 dBm spec SX1262)
  radio.setOutputPower(10);   // reducir para test
  

  uint64_t joinEui64 = euiToU64(JOIN_EUI);
  uint64_t devEui64  = euiToU64(DEV_EUI);

  Serial.print("JOIN_EUI="); printHex(JOIN_EUI, 8);
  Serial.print(" DEV_EUI="); printHex(DEV_EUI, 8);
  Serial.println();

  // TTN LoRaWAN 1.0.x -> APP_KEY en ambos parámetros
  Serial.println("BOOT: node.beginOTAA (config)...");
  st = node.beginOTAA(joinEui64, devEui64, APP_KEY, APP_KEY);
  Serial.print("BOOT: beginOTAA ret = "); Serial.println(st);
  if (st != RADIOLIB_ERR_NONE) return false;

  bool restored = false;

#if FORCE_CLEAN_JOIN
  Serial.println("[RECOVERY] FORCE_CLEAN_JOIN=1 -> erasing SESSION only and forcing OTAA join...");
  nvsEraseSessionOnly();
  nvsRestore(); // aplica nonces si existen
  Serial.println("[RECOVERY] Session erased, nonces applied (if present) ✅");
  restored = false;
#else
  restored = nvsRestore();
#endif

  if (restored) {
    Serial.println("BOOT: Session restored -> activating...");
    st = node.activateOTAA();
    Serial.print("BOOT: activateOTAA (restore) ret = "); Serial.println(st);
    if (loraJoinOk(st)) {
      Serial.println("BOOT: Session active ✅ (skip new join)");
      nvsSave(); // actualiza nonces tras la activación
      return true;
    }
    // Si falla la activación del restore, cae al join normal
    Serial.println("BOOT: Restore activation failed -> forcing new join");
  }

  Serial.println("[BOOT] No session -> proceeding to OTAA (nonces handled by nvsRestore())");
  Serial.println("BOOT: node.activateOTAA...");
  st = node.activateOTAA();
  Serial.print("BOOT: activateOTAA ret = "); Serial.println(st);

  // Guardar nonces SIEMPRE, incluso si falla el join
  nvsSaveNoncesOnly();

  if (loraJoinOk(st)) {
    Serial.println("JOIN OK ✅ -> saving NVS");
    nvsSave();
    return true;
  }

  Serial.println("JOIN FAIL ❌ (device will not uplink)");
  return false;
}

void loraTryJoin() {
  Serial.println("[JOIN] activateOTAA...");
  int16_t st = node.activateOTAA();
  Serial.print("[JOIN] ret="); Serial.println(st);
  if (loraJoinOk(st)) {
    Serial.println("[JOIN] OK ✅ -> saving NVS");
    nvsSave();
  }
}

int16_t loraSend(const uint8_t* payload, size_t len, uint8_t fport) {
  Serial.print("[UP] Sending ("); Serial.print(len); Serial.println("B) on FPort 2...");
  int16_t u = node.sendReceive(payload, len, fport);
  Serial.print("[UP] ret="); Serial.println(u);

  if      (u == 1) Serial.println("[UP] OK (RX1) ✅");
  else if (u == 0) Serial.println("[UP] TX done (no RX) ⚠️");
  else if (u  < 0) Serial.println("[UP] ERROR ❌");

  return u;
}