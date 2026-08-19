#pragma once
#include <TinyGPSPlus.h>

// Objeto GPS accesible desde payload.cpp y main.cpp
extern TinyGPSPlus gps;

// Enciende VEXT, resetea UC6580 e inicia UART1. Llamar en setup().
void gnssInit();

// Vuelca bytes del UART al parser. Llamar en cada iteración del loop().
void gnssFeed();
