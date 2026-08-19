#include "nvs.h"
#include <Arduino.h>
#include <Preferences.h>
#include <RadioLib.h>

// node está definido en lora.cpp
extern LoRaWANNode node;

static Preferences    prefs;
static const char*    NVS_NS     = "lorawan";
static const char*    KEY_MAGIC  = "magic";
static const char*    KEY_NONCES = "nonces";
static const char*    KEY_SESS   = "sess";
static const uint32_t MAGIC      = 0x4C57534E; // 'LWSN'

// =============================================================================

bool nvsRestore() {
  if (!prefs.begin(NVS_NS, false)) {
    Serial.println("[NVS] prefs.begin failed (restore)");
    return false;
  }

  uint32_t m = prefs.getUInt(KEY_MAGIC, 0);
  if (m != MAGIC) {
    prefs.end();
    Serial.println("[NVS] No MAGIC -> no restore");
    return false;
  }

  // --- NONCES ---
  uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
  size_t nNonces = prefs.getBytes(KEY_NONCES, nonces, sizeof(nonces));
  if (nNonces != sizeof(nonces)) {
    Serial.println("[NVS] Nonces missing/size mismatch -> cannot restore");
    prefs.end();
    return false;
  }

  Serial.print("[NVS] Nonces head=");
  for (int i = 0; i < 8; i++) {
    if (nonces[i] < 0x10) Serial.print('0');
    Serial.print(nonces[i], HEX);
    if (i < 7) Serial.print(' ');
  }
  Serial.println();

  // --- SESSION ---
  // Si la key no existe, prefs.getBytesLength puede loguear NOT_FOUND
  size_t sessLen = prefs.getBytesLength(KEY_SESS);
  if (sessLen == 0) {
    Serial.println("[NVS] Session key missing (sess NOT_FOUND) -> applying NONCES only");
    int16_t r1 = node.setBufferNonces(nonces);
    Serial.print("[NVS] setBufferNonces ret="); Serial.println(r1);
    prefs.end();
    // No hay sesión -> no podemos activar, pero dejamos nonces listos para OTAA
    return false;
  }

  if (sessLen != RADIOLIB_LORAWAN_SESSION_BUF_SIZE) {
    Serial.print("[NVS] Session size mismatch: ");
    Serial.print(sessLen);
    Serial.print(" != ");
    Serial.println(RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
    prefs.remove(KEY_SESS);
    prefs.end();
    Serial.println("[NVS] Deleted invalid session key");
    return false;
  }

  uint8_t sess[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
  size_t nSess = prefs.getBytes(KEY_SESS, sess, sizeof(sess));
  prefs.end();

  if (nSess != sizeof(sess)) {
    Serial.println("[NVS] Session read mismatch -> restore failed");
    return false;
  }

  int16_t r1 = node.setBufferNonces(nonces);
  int16_t r2 = node.setBufferSession(sess);
  Serial.print("[NVS] setBufferNonces ret="); Serial.println(r1);
  Serial.print("[NVS] setBufferSession ret="); Serial.println(r2);

  if (r1 == RADIOLIB_ERR_NONE && r2 == RADIOLIB_ERR_NONE) {
    Serial.println("[NVS] Restore OK ✅");
    return true;
  }

  Serial.println("[NVS] Restore failed -> ignoring");
  return false;
}

// =============================================================================

void nvsSave() {
  uint8_t* nonces = node.getBufferNonces();
  uint8_t* sess   = node.getBufferSession();

  if (!nonces || !sess) {
    Serial.println("[NVS] getBufferNonces/session returned NULL -> not saving");
    return;
  }

  if (!prefs.begin(NVS_NS, false)) {
    Serial.println("[NVS] prefs.begin failed (save)");
    return;
  }

  prefs.putUInt(KEY_MAGIC, MAGIC);
  prefs.putBytes(KEY_NONCES, nonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  prefs.putBytes(KEY_SESS,   sess,   RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  prefs.end();

  Serial.println("[NVS] Saved session+nonces ✅");
}

// =============================================================================

void nvsSaveNoncesOnly() {
  uint8_t* nonces = node.getBufferNonces();
  if (!nonces) {
    Serial.println("[NVS] getBufferNonces returned NULL");
    return;
  }

  if (!prefs.begin(NVS_NS, false)) {
    Serial.println("[NVS] prefs.begin failed (save nonces)");
    return;
  }

  // Asegura que exista magic para que nvsRestore() funcione
  prefs.putUInt(KEY_MAGIC, MAGIC);

  Serial.print("[NVS] Nonces head=");
  for (int i = 0; i < 8; i++) {
    if (nonces[i] < 0x10) Serial.print('0');
    Serial.print(nonces[i], HEX);
    if (i < 7) Serial.print(' ');
  }
  Serial.println();

  size_t wr = prefs.putBytes(KEY_NONCES, nonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  prefs.end();

  Serial.print("[NVS] putBytes(KEY_NONCES) wr="); Serial.println(wr);
  Serial.println("[NVS] Saved nonces ✅");
}

// =============================================================================

void nvsEraseSessionOnly() {
  if (!prefs.begin(NVS_NS, false)) {
    Serial.println("[NVS] prefs.begin failed (erase session)");
    return;
  }
  prefs.remove(KEY_SESS);
  // MUY IMPORTANTE: NO borrar KEY_NONCES
  prefs.end();
  Serial.println("[NVS] Erased session only (kept nonces) ✅");
}
