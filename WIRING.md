# Conexiones hardware — HTIT-Tracker + sensores externos

Hardware base: **Heltec HTIT-Tracker V1.2** (ESP32-S3)

---

## Bus SPI — LoRa SX1262 (interno, no modificar)

| Señal | GPIO ESP32-S3 |
|-------|--------------|
| NSS (CS) | 8 |
| SCK      | 9 |
| MOSI     | 10 |
| MISO     | 11 |
| RST      | 12 |
| BUSY     | 13 |
| DIO1     | 14 |

---

## GNSS UC6580 (interno, UART1, solo RX)

| Señal | GPIO ESP32-S3 |
|-------|--------------|
| VEXT CTRL | 3 |
| RST       | 35 |
| RX (solo) | 33 |
| Baudios   | 115200 |

---

## MAX31865 — PT100 3 hilos (SPI externo)

### Configuración del módulo MAX31865

Antes de conectar, verificar que los jumpers de soldadura estén así:

| Jumper | Estado |
|--------|--------|
| `2/3 Wire` | **CERRADO** (soldado) |
| Pad `24` ↔ central | **ABIERTO** (cortado/raspado) |
| Pad central ↔ `3` | **CERRADO** (soldado) |

> ⚠️ Si estos jumpers no están correctamente configurados, el sensor
> leerá valores erróneos aunque el cableado externo sea perfecto.

### Conexión SPI: MAX31865 ↔ ESP32-S3

| Pin MAX31865 | GPIO ESP32-S3 | Cable |
|--------------|--------------|-------|
| VIN          | 3V3          | Rojo |
| GND          | GND          | Negro |
| CLK          | GPIO 5       | Amarillo |
| SDO (MISO)   | GPIO 6       | Verde |
| SDI (MOSI)   | GPIO 7       | Azul |
| CS           | GPIO 4       | Marrón |
| RDY/DRDY     | sin conectar (polling) | — |

> El bus SPI del MAX31865 es independiente del SPI interno del SX1262.
> El ESP32-S3 soporta múltiples buses SPI simultáneos.

### Conexión PT100 3 hilos ↔ MAX31865

```
PT100 (3 hilos)          Terminal MAX31865
─────────────────────────────────────────
Hilo A  (extremo único)  →  RTD−
Hilo B1 (extremo doble)  →  F+
Hilo B2 (extremo doble)  →  RTD+
                             F−  ←── puente interno al seleccionar modo 3W
```

Esquema visual:

```
           ┌──────────────┐
           │   Elemento   │
           │   PT100      │
           └──┬───────────┘
              │  extremo A (1 hilo)  →  RTD−
              │
           ┌──┴───────────┐
           │   Elemento   │
           │   PT100      │
           └──┬──┬────────┘
              │  └─────────────────────  RTD+
              │    extremo B (2 hilos)
              └───────────────────────   F+
```

> Los dos hilos del extremo B van cada uno a un terminal distinto
> (F+ y RTD+). Esta es la clave del modo 3 hilos: el MAX31865
> mide la resistencia del cable y la compensa automáticamente.

---

## MS5611 — Presión / Temperatura (I2C)

Reemplaza al BMP280 desde v6.0. Mayor rango (10–1200 hPa) y resolución (24-bit ADC).

| Pin MS5611 | GPIO ESP32-S3 | Nota |
|------------|--------------|------|
| VCC | 3V3 | |
| GND | GND | |
| SCL | GPIO 18 | |
| SDA | GPIO 17 | |
| CSB | 3V3 | dirección I2C 0x76 |
| SDO | GND | sin función en I2C |
| PS  | 3V3 | fuerza modo I2C |

> ⚠️ El pin PS es exclusivo del MS5611 y no existe en el BMP280.
> Sin él conectado a 3V3 el sensor no responde por I2C.

---

## Batería (interno)

| Señal | GPIO ESP32-S3 |
|-------|--------------|
| VBAT_READ (ADC) | GPIO 1 |
| ADC_CTRL        | GPIO 2 |

---

## Resumen de GPIOs ocupados

| GPIO | Función |
|------|---------|
| 1  | Batería ADC |
| 2  | Batería CTRL |
| 3  | VEXT CTRL (GNSS) |
| 4  | MAX31865 CS |
| 5  | MAX31865 SCK |
| 6  | MAX31865 MISO |
| 7  | MAX31865 MOSI |
| 8  | LoRa NSS |
| 9  | LoRa SCK |
| 10 | LoRa MOSI |
| 11 | LoRa MISO |
| 12 | LoRa RST |
| 13 | LoRa BUSY |
| 14 | LoRa DIO1 |
| 17 | MS5611 SDA |
| 18 | MS5611 SCL |
| 33 | GNSS RX |
| 35 | GNSS RST |
