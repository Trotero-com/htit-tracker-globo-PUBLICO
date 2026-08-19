#include "gnss.h"
#include "config.h"
#include <Arduino.h>

TinyGPSPlus gps;
static HardwareSerial GNSS(1); // UART1

void gnssInit() {
  pinMode(PIN_VEXT_CTRL, OUTPUT);
  digitalWrite(PIN_VEXT_CTRL, HIGH);
  delay(200);

  pinMode(PIN_GNSS_RST, OUTPUT);
  digitalWrite(PIN_GNSS_RST, HIGH);
  delay(20);
  digitalWrite(PIN_GNSS_RST, LOW);
  delay(50);
  digitalWrite(PIN_GNSS_RST, HIGH);
  delay(200);

  // NMEA entra por PIN_GNSS_RX_ONLY, solo RX
  GNSS.begin(GNSS_BAUD, SERIAL_8N1, PIN_GNSS_RX_ONLY, -1);
}

void gnssFeed() {
  while (GNSS.available()) gps.encode(GNSS.read());
}
