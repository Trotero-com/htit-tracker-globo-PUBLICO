#pragma once
#include <RadioLib.h>

// Objeto nodo accesible desde nvs.cpp y main.cpp
extern LoRaWANNode node;

// Devuelve true si el código de retorno indica join exitoso.
bool loraJoinOk(int16_t st);

// Inicializa SPI, radio y configura OTAA. Intenta restaurar sesión desde NVS.
// Devuelve true si el nodo quedó activado.
bool loraInit();

// Intenta activateOTAA. Guarda nonces (y sesión si OK).
// Usar en el loop() para reintentos periódicos.
void loraTryJoin();

// Envía payload por LoRaWAN e imprime el resultado.
// Devuelve el código de retorno de sendReceive().
int16_t loraSend(const uint8_t* payload, size_t len, uint8_t fport);
