// ============================================================
// TTN Payload Decoder — tracker-globo
// Protocolo: 1 muestra completa (20B) + N mini-muestras (11B c/u)
//
// Muestra completa (20 bytes, big-endian):
//   [0]     t_offset_s  uint8   segundos antes del uplink
//   [1]     fix+sats    uint8   bit7=fix, bits0-6=sats
//   [2-4]   lat         int24   grados × 1e5
//   [5-7]   lon         int24   grados × 1e5
//   [8-9]   alt_m       uint16  metros
//   [10-11] speed_mps   uint16  cm/s → ÷100 → m/s
//   [12-13] temp_ext_c  int16   °C × 10
//   [14-15] temp_int_c  int16   °C × 10
//   [16-17] pres_hpa    uint16  hPa × 10
//   [18-19] batt_raw    uint16  ADC 12-bit (0–4095)
//
// Mini-muestra (11 bytes, big-endian):
//   [0]     t_offset_s  uint8   segundos antes del uplink
//   [1-3]   lat         int24   grados × 1e5
//   [4-6]   lon         int24   grados × 1e5
//   [7-8]   alt_m       uint16  metros
//   [9-10]  speed_mps   uint16  cm/s → ÷100 → m/s
// ============================================================

function decodeUplink(input) {
  var bytes = input.bytes;
  var FULL  = 20;
  var MINI  = 11;

  if (bytes.length < FULL) {
    return {
      data:   {},
      errors: ["Payload demasiado corto: " + bytes.length + "B, mínimo " + FULL + "B"]
    };
  }

  // ── Helpers de lectura big-endian ───────────────────────

  function u16(i) {
    return (bytes[i] << 8) | bytes[i + 1];
  }

  function i16(i) {
    var v = u16(i);
    return v >= 0x8000 ? v - 0x10000 : v;
  }

  // Sign-extend 24 bits → JavaScript number (IEEE 754 double)
  function i24(i) {
    var v = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
    return v >= 0x800000 ? v - 0x1000000 : v;
  }

  // ── Muestra completa (bytes 0–19) ───────────────────────

  var fs = bytes[1];

  var full = {
    t_offset_s : bytes[0],
    fix        : (fs & 0x80) !== 0,
    sats       :  fs & 0x7F,
    lat        : i24(2) / 1e5,
    lon        : i24(5) / 1e5,
    alt_m      : u16(8),
    speed_mps  : u16(10) / 100,
    temp_ext_c : i16(12) / 10,
    temp_int_c : i16(14) / 10,
    pres_hpa   : u16(16) / 10,
    batt_raw   : u16(18)
  };

  // ── Mini-muestras (bytes 20 en adelante) ────────────────

  var minis  = [];
  var offset = FULL;

  while (offset + MINI <= bytes.length) {
    minis.push({
      t_offset_s : bytes[offset],
      lat        : i24(offset + 1) / 1e5,
      lon        : i24(offset + 4) / 1e5,
      alt_m      : u16(offset + 7),
      speed_mps  : u16(offset + 9) / 100
    });
    offset += MINI;
  }

  // ── Resultado — nombres de la tabla, estructura plana ──

  var warnings = [];
  if (!full.fix) warnings.push("Muestra completa sin fix GPS");
  if (bytes.length > FULL && (bytes.length - FULL) % MINI !== 0) {
    warnings.push("Bytes sobrantes al final: " + ((bytes.length - FULL) % MINI) + "B");
  }

  return {
    data: {
      // Muestra completa — campos de la tabla
      altitud   : full.alt_m,
      lat       : full.lat,
      lon       : full.lon,
      fix       : full.fix,
      sats      : full.sats,
      speed_gps : full.speed_mps,
      temp_ext  : full.temp_ext_c,
      temp_int  : full.temp_int_c,
      presion   : full.pres_hpa,
      batt_raw  : full.batt_raw,

      // Mini-muestras intermedias (trayectoria)
      // Cada una: { t_offset_s, lat, lon, altitud, speed_gps }
      minis  : minis.map(function(m) {
        return {
          t_offset_s : m.t_offset_s,
          lat        : m.lat,
          lon        : m.lon,
          altitud    : m.alt_m,
          speed_gps  : m.speed_mps
        };
      }),
      n_minis: minis.length
    },

    // Ubicación para el mapa de TTN console — solo si hay fix GPS
    locations: full.fix ? {
      user: {
        latitude : full.lat,
        longitude: full.lon,
        altitude : full.alt_m,
        source   : "GPS"
      }
    } : undefined,

    warnings: warnings,
    errors  : []
  };
}
