# tracker-globo

Tracker GPS \+ telemetría LoRaWAN para globo estratosférico.  
Hardware base: **Heltec HTIT-Tracker V1.2** (ESP32-S3).  
Red: **TTN OTAA EU868** — gateway propio `trotero`.

---

## Configuración inicial — credenciales TTN

Este repositorio **no incluye claves reales**. Antes de compilar:

1. Copia la plantilla:

   `include/credentials.h.example`  ->  `include/credentials.h`

2. Rellena `credentials.h` con los valores de tu dispositivo, que encontrarás en
   la consola de TTN en *Applications -> [tu app] -> End devices -> [tu device]*.
   Cópialos en formato **MSB-first (big-endian)**.

3. No toques nada más: `include/credentials.h` está en `.gitignore`, de modo que
   tus claves no pueden subirse al repositorio por accidente.

> **Seguridad:** la *AppKey* es la clave raíz del dispositivo. Si alguna vez se
> ha publicado, no basta con borrarla del código — hay que regenerarla en TTN.

---

## Hardware

| Componente | Función | Interfaz |
| :---- | :---- | :---- |
| ESP32-S3 (Heltec HTIT-Tracker V1.2) | MCU principal | — |
| SX1262 (interno) | Radio LoRa | SPI0 (GPIOs 8–14) |
| UC6580 (interno) | GNSS multi-constelación | UART1 (GPIO 33\) |
| MAX31865 | PT100 temperatura exterior | SPI1 (GPIOs 4–7) |
| BMP280 | Temperatura \+ presión interior | I2C (GPIOs 17–18) |
| LiPo (interno) | Batería | ADC GPIO 1 |

El cableado detallado de cada componente está en **WIRING.md**.

---

## Campos de telemetría

Enviados en cada uplink (mínimo una vez cada 30 s):

| \# | Campo | Unidad | Fuente |
| :---- | :---- | :---- | :---- |
| 1 | `altitud` | m | UC6580 GPS |
| 2 | `lat` | ° | UC6580 GPS |
| 3 | `lon` | ° | UC6580 GPS |
| 4 | `sats` | — | UC6580 GPS |
| 5 | `speed_gps` | m/s | UC6580 GPS |
| 6 | `temp_ext` | °C | MAX31865 PT100 |
| 7 | `temp_int` | °C | BMP280 |
| 8 | `presion` | hPa | BMP280 |
| 9 | `batt_raw` | ADC | ESP32-S3 ADC 12-bit |

---

## Arquitectura del sistema (v4.0)

### Flujo de datos

Sensores → sampler (cada 3s) → buffer circular (30 slots)

                                        ↓

                              buildPacket() cada 30s

                                        ↓

                              loraSend() → TTN → Decoder JS

### Protocolo de paquete

Cada uplink contiene **1 muestra completa \+ N mini-muestras**, donde N depende del Data Rate actual (`node.getMaxPayloadLen()`).

#### Muestra completa — 20 bytes (big-endian)

| Bytes | Campo | Tipo | Encoding |
| :---- | :---- | :---- | :---- |
| 0 | t\_offset\_s | uint8 | segundos antes del uplink |
| 1 | fix \+ sats | uint8 | bit7=fix, bits0-6=sats |
| 2–4 | lat | int24 | grados × 1e5 |
| 5–7 | lon | int24 | grados × 1e5 |
| 8–9 | altitud | uint16 | metros |
| 10–11 | speed\_gps | uint16 | cm/s (÷100 → m/s) |
| 12–13 | temp\_ext | int16 | °C × 10 |
| 14–15 | temp\_int | int16 | °C × 10 |
| 16–17 | presion | uint16 | hPa × 10 |
| 18–19 | batt\_raw | uint16 | ADC 12-bit |

#### Mini-muestra — 11 bytes (big-endian)

| Bytes | Campo | Tipo | Encoding |
| :---- | :---- | :---- | :---- |
| 0 | t\_offset\_s | uint8 | segundos antes del uplink |
| 1–3 | lat | int24 | grados × 1e5 |
| 4–6 | lon | int24 | grados × 1e5 |
| 7–8 | altitud | uint16 | metros |
| 9–10 | speed\_gps | uint16 | cm/s (÷100 → m/s) |

Solo se incluyen mini-muestras con fix GPS válido.

#### Capacidad por Data Rate (EU868)

| DR | SF | Max payload | Mini-muestras |
| :---- | :---- | :---- | :---- |
| DR5/4 | SF7/8 | 222B | hasta 18 |
| DR3 | SF9 | 115B | hasta 8 |
| DR2 | SF10 | 51B | hasta 2 |

### Buffer circular

- **30 slots** — cubre 90 s de historial ante fallo de uplink  
- Si el buffer está lleno, descarta la muestra más antigua  
- Tras uplink exitoso, libera solo las muestras enviadas  
- Las muestras no enviadas permanecen para el próximo ciclo

---

## Estructura de ficheros

include/

  config.h        Pines, claves TTN, constantes de comportamiento

  credentials.h   JOIN\_EUI, DEV\_EUI, APP\_KEY  ← EN .gitignore, NO subir

  credentials.h.example   Plantilla sin claves — esta sí se publica

  gnss.h          Driver UC6580

  lora.h          Radio SX1262, join, uplink

  nvs.h           Persistencia sesión LoRaWAN en NVS

  sampler.h       Buffer circular, struct Sample

  payload.h       buildPacket(), printPacket()

  battery.h       Lectura ADC batería

  bme280.h        Driver BMP280

  max31865.h      Driver MAX31865 PT100

  utils.h         Utilidades (euiToU64, printHex)

src/

  main.cpp        setup() y loop()

  gnss.cpp

  lora.cpp

  nvs.cpp

  sampler.cpp     Captura cada 3s, buffer circular

  payload.cpp     buildPacket() DR-aware, printPacket()

  battery.cpp

  bme280.cpp

  max31865.cpp

ttn\_decoder.js    Decoder JavaScript para TTN Payload Formatter

WIRING.md         Conexiones hardware detalladas

README.md         Este fichero

---

## Constantes clave (`config.h`)

| Constante | Valor | Descripción |
| :---- | :---- | :---- |
| `SAMPLE_INTERVAL_MS` | 3000 | Captura de sensores cada 3 s |
| `UPLINK_INTERVAL_MS` | 30000 | Uplink LoRaWAN cada 30 s |
| `JOIN_RETRY_MS` | 60000 | Reintento de join si sin sesión |
| `FORCE_CLEAN_JOIN` | 0 | Poner a 1 solo para recuperar desincronización TTN |
| `VALIDATE_SESSION_AFTER_RESTORE` | 0 | Validación de sesión restaurada |
| `WATCHDOG_TIMEOUT_S` | 60 | Reinicio automático si loop bloqueado |

---

## TTN — Payload Formatter

Fichero: `ttn_decoder.js`  
Ruta en TTN: `Applications → tracker-globo → Payload formatters → Uplink → Custom Javascript`

Salida decodificada:

{

  "altitud"  : 647,

  "lat"      : 40.00418,

  "lon"      : \-3.57523,

  "fix"      : true,

  "sats"     : 18,

  "speed\_gps": 0.05,

  "temp\_ext" : 19.5,

  "temp\_int" : 20.3,

  "presion"  : 949.2,

  "batt\_raw" : 889,

  "minis"    : \[ { "t\_offset\_s": 23, "lat": ..., "lon": ..., "altitud": ..., "speed\_gps": ... }, ... \],

  "n\_minis"  : 7

}

---

## Stack tecnológico

| Componente | Versión |
| :---- | :---- |
| PlatformIO \+ Arduino | — |
| espressif32 | ESP32-S3 |
| RadioLib | 7.3.0 |
| TinyGPSPlus | 1.1.0 |
| Adafruit BMP280 | 2.6.8 |
| Adafruit MAX31865 | — |

---

## Historial de versiones

| Tag | Descripción |
| :---- | :---- |
| v5.0 | Watchdog hardware 60s + nvsSave() tras cada uplink. Robustez para vuelo autónomo |
| v4.0 | Arquitectura multi-muestra para globo estratosférico. Sampler 3s, paquete DR-aware (full+mini), decoder TTN |
| v3.0 | Payload 24B: GNSS \+ BMP280 \+ PT100 \+ batería. Join OTAA \+ persistencia NVS |


