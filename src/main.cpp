#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "gnss.h"
#include "lora.h"
#include "nvs.h"
#include "sampler.h"
#include "payload.h"
#include "pressure.h"
#include "max31865.h"

// Estado de validación de sesión restaurada (sin cambios)
#if VALIDATE_SESSION_AFTER_RESTORE
static bool needSessionValidation = false;
static bool sessionValidationDone = false;
#endif

// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(5500);

  Serial.println();
  Serial.println("BOOT: tracker-globo");

  gnssInit();
  Serial.println("BOOT: GNSS OK");

  pressureInit();
  max31865Init();

  bool restored = loraInit();

  #if VALIDATE_SESSION_AFTER_RESTORE
    if (restored) {
      needSessionValidation = true;
      sessionValidationDone = false;
      Serial.println("[SESSION] Will validate restored session with 1 CONFIRMED uplink");
    }
  #endif
 
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  samplerInit();    // Iniciar sampler AL FINAL de setup — sensores ya inicializados
  Serial.println("BOOT: Sampler + Watchdog OK ✅ (60s)");
}

// =============================================================================
void loop() {
  esp_task_wdt_reset();   // "sigo vivo"
  static uint32_t lastUp = 0;   // declarado aquí para ser accesible en el bloque de apogeo

  // ── Llamadas que deben ejecutarse SIEMPRE, en cada iteración ──
  gnssFeed();
  samplerTick();        // captura muestra si han pasado SAMPLE_INTERVAL_MS
  
  // 1️⃣ Reintento join cada JOIN_RETRY_MS si no activado
  if (!node.isActivated()) {
    static uint32_t lastJoin = 0;
    if (millis() - lastJoin > JOIN_RETRY_MS) {
      lastJoin = millis();
      loraTryJoin();
    }
    delay(50);
    return;
    // Nota: el sampler sigue acumulando muestras durante la espera del join.
    // Cuando el join tenga éxito, el primer uplink enviará el historial completo.
  }

  // 2️⃣ Validación de sesión restaurada (una sola vez tras restore)
#if VALIDATE_SESSION_AFTER_RESTORE
  if (needSessionValidation && !sessionValidationDone) {
    sessionValidationDone = true;

    uint8_t v = 0x00;
    Serial.println("[SESSION] Validating restored session: 1B CONFIRMED uplink...");
    int16_t w = node.sendReceive(&v, 1, SESSION_VALIDATE_FPORT,
                                 nullptr, nullptr,
                                 true); // confirmed = true
    Serial.print("[SESSION] ret="); Serial.println(w);

    if (w >= 0) {
      Serial.println("[SESSION] Uplink accepted ✅ -> session VALID");
      needSessionValidation = false;
    } else {
      Serial.println("[SESSION] Error ❌ -> session INVALID");
      Serial.println("[SESSION] Erasing session (keep nonces) and restarting...");
      nvsEraseSessionOnly();
      delay(200);
      ESP.restart();
    }
    delay(50);
    return;
  }
#endif

  // 3️⃣ Uplink periódico
  if (millis() - lastUp > UPLINK_INTERVAL_MS) {
    lastUp = millis();

    uint8_t pktBuf[PKT_BUF_MAX];
    uint8_t sentMinis = 0;
    uint8_t maxLen    = node.getMaxPayloadLen();

    size_t pktLen = buildPacket(pktBuf, maxLen, &sentMinis);

    if (pktLen == 0) {
      Serial.println("[UP] Sin muestras en buffer — uplink omitido");
      return;
    }

    printPacket(pktBuf, pktLen);

    int16_t st = loraSend(pktBuf, pktLen, 2);

    if (st >= 0) {
      // Liberar del buffer solo las muestras que se han enviado:
      // 1 muestra completa + sentMinis mini-muestras
      samplerClear(1 + sentMinis);
      nvsSave();   // ← añadir esta línea para guardar el nuevo estado del buffer tras el envío
    }
    // Si el uplink falla, las muestras permanecen en el buffer
    // y se intentarán enviar en el próximo ciclo.
  }

  delay(20);
}
