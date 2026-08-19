#include <Arduino.h>
#include "config.h"
#include "gnss.h"
#include "lora.h"
#include "nvs.h"
#include "payload.h"

// Estado de validación de sesión restaurada
#if VALIDATE_SESSION_AFTER_RESTORE
static bool needSessionValidation = false;
static bool sessionValidationDone = false;
#endif

// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(5500);

  Serial.println();
  Serial.println("BOOT: OTAA + GNSS payload");

  gnssInit();
  Serial.println("BOOT: GNSS UART OK");

  bool restored = loraInit();

#if VALIDATE_SESSION_AFTER_RESTORE
  if (restored) {
    needSessionValidation = true;
    sessionValidationDone = false;
    Serial.println("[SESSION] Will validate restored session with 1 CONFIRMED uplink");
  }
#endif
}

// =============================================================================
void loop() {
  gnssFeed();

  // Reintento join cada 60s si no activado
  if (!node.isActivated()) {
    static uint32_t lastJoin = 0;
    if (millis() - lastJoin > 60000UL) {
      lastJoin = millis();
      loraTryJoin();
    }
    delay(50);
    return;
  }

  // Validación de sesión restaurada (una sola vez)
#if VALIDATE_SESSION_AFTER_RESTORE
  if (needSessionValidation && !sessionValidationDone) {
    sessionValidationDone = true;

    uint8_t v = 0x00;
    Serial.println("[SESSION] Validating restored session: 1B CONFIRMED uplink...");

    // Firma exacta del original que funciona
    int16_t w = node.sendReceive(&v, 1, SESSION_VALIDATE_FPORT,
                                 nullptr, nullptr,
                                 true); // confirmed = true
    Serial.print("[SESSION] ret="); Serial.println(w);

    if (w > 0) {
      Serial.println("[SESSION] ACK received ✅ -> session VALID");
      needSessionValidation = false;
    } else {
      Serial.println("[SESSION] No ACK ❌ -> session INVALID in TTN");
      Serial.println("[SESSION] Erasing stored session (keep nonces) and restarting...");
      nvsEraseSessionOnly();
      delay(200);
      ESP.restart();
    }

    delay(50);
    return;
  }
#endif

  // Uplink periódico
  static uint32_t lastUp = 0;
  if (millis() - lastUp > UPLINK_INTERVAL_MS) {
    lastUp = millis();

    uint8_t payload[PAYLOAD_SIZE];
    buildPayload(payload);

    Serial.print("[UP] bytes="); Serial.println(PAYLOAD_SIZE);
    printPayload(payload);

    loraSend(payload, PAYLOAD_SIZE, 2);
  }

  delay(20);
}
