#include "payload.h"
#include "battery.h"
#include "gnss.h"
#include <Arduino.h>

void buildPayload(uint8_t out[PAYLOAD_SIZE]) {
  uint8_t fix  = gps.location.isValid() ? 1 : 0;
  uint8_t sats = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;

  int32_t  lat         = 0;
  int32_t  lon         = 0;
  int16_t  alt         = 0;
  uint16_t speed_cms   = 0;
  uint16_t course_cdeg = 0;

  uint16_t batt_raw = readBatteryRaw();

  if (gps.location.isValid()) {
    lat = (int32_t)(gps.location.lat() * 1e7);
    lon = (int32_t)(gps.location.lng() * 1e7);
  }

  if (gps.altitude.isValid()) {
    alt = (int16_t)gps.altitude.meters();
  }

  // Solo tiene sentido con fix válido (evita basura cuando no hay posición)
  if (fix) {
    if (gps.speed.isValid()) {
      double v = gps.speed.mps();
      if (v < 0) v = 0;
      double cms = v * 100.0;
      if (cms > 65535.0) cms = 65535.0;
      speed_cms = (uint16_t)(cms + 0.5);
    }

    if (gps.course.isValid()) {
      double c = gps.course.deg();
      if (c < 0) c = 0;
      if (c > 360.0) c = 360.0;
      double cdeg = c * 100.0;
      if (cdeg > 65535.0) cdeg = 65535.0;
      course_cdeg = (uint16_t)(cdeg + 0.5);
    }
  }

  out[0] = fix;
  out[1] = sats;

  out[2] = (uint8_t)((lat >> 24) & 0xFF);
  out[3] = (uint8_t)((lat >> 16) & 0xFF);
  out[4] = (uint8_t)((lat >>  8) & 0xFF);
  out[5] = (uint8_t)( lat        & 0xFF);

  out[6] = (uint8_t)((lon >> 24) & 0xFF);
  out[7] = (uint8_t)((lon >> 16) & 0xFF);
  out[8] = (uint8_t)((lon >>  8) & 0xFF);
  out[9] = (uint8_t)( lon        & 0xFF);

  out[10] = (uint8_t)((alt >> 8) & 0xFF);
  out[11] = (uint8_t)( alt       & 0xFF);

  out[12] = (uint8_t)((speed_cms >> 8) & 0xFF);
  out[13] = (uint8_t)( speed_cms       & 0xFF);

  out[14] = (uint8_t)((course_cdeg >> 8) & 0xFF);
  out[15] = (uint8_t)( course_cdeg       & 0xFF);

  out[16] = (uint8_t)((batt_raw >> 8) & 0xFF);
  out[17] = (uint8_t)( batt_raw       & 0xFF);
}

void printPayload(const uint8_t* p) {
  // Hex dump
  Serial.print("[UP] hex=");
  for (size_t i = 0; i < PAYLOAD_SIZE; i++) {
    if (p[i] < 0x10) Serial.print('0');
    Serial.print(p[i], HEX);
    if (i + 1 < PAYLOAD_SIZE) Serial.print(' ');
  }
  Serial.println();

  // Decodificación legible
  int32_t  lat         = ((int32_t)p[2]  << 24) | ((int32_t)p[3]  << 16) | ((int32_t)p[4] << 8) | p[5];
  int32_t  lon         = ((int32_t)p[6]  << 24) | ((int32_t)p[7]  << 16) | ((int32_t)p[8] << 8) | p[9];
  int16_t  alt         = ((int16_t)p[10] <<  8) | p[11];
  uint16_t speed_cms   = ((uint16_t)p[12] << 8) | p[13];
  uint16_t course_cdeg = ((uint16_t)p[14] << 8) | p[15];
  uint16_t batt_raw    = ((uint16_t)p[16] << 8) | p[17];

  // Si va prácticamente parado, el rumbo GNSS no es fiable
  if (speed_cms < 50) course_cdeg = 0;

  Serial.print("[GNSS] fix=");    Serial.print(p[0]);
  Serial.print(" sats=");         Serial.print(p[1]);
  Serial.print(" lat=");          Serial.print(lat / 1e7, 7);
  Serial.print(" lon=");          Serial.print(lon / 1e7, 7);
  Serial.print(" alt=");          Serial.println(alt);
  Serial.print(" speed=");        Serial.print(speed_cms / 100.0);   Serial.print(" m/s");
  Serial.print(" course=");       Serial.print(course_cdeg / 100.0); Serial.println(" deg");
  Serial.print(" batt_raw=");     Serial.println(batt_raw);
}
