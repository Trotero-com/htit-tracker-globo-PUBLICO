#pragma once

// Restaura nonces + sesión desde NVS.
// Devuelve true si la sesión quedó completamente restaurada y activable.
bool nvsRestore();

// Guarda nonces + sesión completa en NVS.
void nvsSave();

// Guarda solo los nonces (llamar siempre tras activateOTAA, aunque falle).
void nvsSaveNoncesOnly();

// Borra únicamente la clave de sesión. NO toca los nonces.
void nvsEraseSessionOnly();
