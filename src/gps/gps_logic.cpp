#include <Arduino.h>
#include "gps_logic.h"
#include <TinyGPSPlus.h>
#include <math.h>
#include <string.h>
#include "../user_logic.h"  // Pin mapping (GPS_RX_PIN / GPS_TX_PIN)

// Implementación segura y no bloqueante del GPS para Custodia.
// - Usa UART hardware en nRF52 (Serial1) cuando existe.
// - No usa SleepyDog ni deep sleep.
// - Siempre publica datos aunque no haya fix (lat/lon = NAN).

// =================== Configuración ===================
#ifndef GPS_BAUD
#define GPS_BAUD 9600
#endif

#ifndef GPS_STREAM_BUDGET
#define GPS_STREAM_BUDGET 1024  // bytes por llamada (aumentado para drenar ráfagas)
#endif

#ifndef GPS_DEBUG_INTERVAL_MS
#define GPS_DEBUG_INTERVAL_MS 5000
#endif

// =================== Backend serial ===================
#if defined(ARDUINO_ARCH_ESP32)
  #include <HardwareSerial.h>
  static HardwareSerial GPSSerial(1);
  #define GPS_HAVE_HW_SERIAL 1
#elif defined(ARDUINO_ARCH_NRF52) && defined(NRF52_USE_SERIAL1)
  // En nRF52, activar Serial1 (UART HW) solo si se define NRF52_USE_SERIAL1 explícitamente
  #define GPS_HAVE_HW_SERIAL 1
#else
  #include <SoftwareSerial.h>
  static SoftwareSerial GPSSerial(GPS_RX_PIN, GPS_TX_PIN);
#endif

// =================== Estado global ===================
GPSData gpsData = {DEFAULT_LATITUDE, DEFAULT_LONGITUDE, false, 0, 0};
GPSStatus gpsStatus = GPS_STATUS_OFF;

static TinyGPSPlus parser;
static bool serialReady = false;
static unsigned long lastDebug = 0;
static uint32_t currentBaud = GPS_BAUD;

// Autobaud simple: probar varios baudios hasta detectar NMEA plausibles
#ifndef GPS_AUTOBAUD_ENABLED
#define GPS_AUTOBAUD_ENABLED 0 // Desactivado: Falla y causa falsos positivos. 9600 bps es la velocidad correcta.
#endif
#ifndef GPS_AUTOBAUD_WINDOW_MS
#define GPS_AUTOBAUD_WINDOW_MS 1500
#endif
#if GPS_AUTOBAUD_ENABLED
static const uint32_t kBaudCandidates[] = {9600, 38400, 115200};
static const size_t kBaudCount = sizeof(kBaudCandidates) / sizeof(kBaudCandidates[0]);
static size_t autobaudIndex = 0;
static bool autobaudLocked = false;
static unsigned long autobaudStartMs = 0;
static const size_t NMEA_SNIFF_MAX = 256;
static char nmeaSniffBuf[NMEA_SNIFF_MAX];
static size_t nmeaSniffLen = 0;
#endif

// =================== Utilidades ===================
static inline Stream& gpsStream() {
#if defined(ARDUINO_ARCH_ESP32)
  return GPSSerial;
#elif defined(ARDUINO_ARCH_NRF52) && defined(NRF52_USE_SERIAL1)
  return Serial1;
#else
  return GPSSerial;
#endif
}

static void beginSerialAt(uint32_t baud) {
#if defined(ARDUINO_ARCH_ESP32)
  GPSSerial.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#elif defined(ARDUINO_ARCH_NRF52) && defined(NRF52_USE_SERIAL1)
  Serial1.begin(baud);
#else
  GPSSerial.begin(baud);
  GPSSerial.listen();
#endif
  currentBaud = baud;
}

static void ensureSerial() {
  if (serialReady) return;

  beginSerialAt(GPS_BAUD);
  serialReady = true;
}

static void updateSnapshot() {
  const bool valid = parser.location.isValid();
  gpsData.latitude = valid ? parser.location.lat() : NAN;
  gpsData.longitude = valid ? parser.location.lng() : NAN;
  gpsData.hasValidFix = valid;
  gpsData.satellites = parser.satellites.isValid() ? (uint8_t)parser.satellites.value() : 0;
  gpsData.timestamp = millis() / 1000;
  gpsStatus = valid ? GPS_STATUS_FIX_3D : GPS_STATUS_SEARCHING;
}

static void debugPrint() {
  const unsigned long now = millis();
  if (now - lastDebug < GPS_DEBUG_INTERVAL_MS) return;
  lastDebug = now;

  String line = "[GPS] ";
  if (parser.location.isValid()) {
    line += String(parser.location.lat(), 6);
    line += ",";
    line += String(parser.location.lng(), 6);
  } else {
    line += "INVALID,INVALID";
  }
  line += " | sats=";
  line += parser.satellites.isValid() ? String(parser.satellites.value()) : String(0);
  line += " | chars=";
  line += String(parser.charsProcessed());
  line += " | fix_sentences=";
  line += String(parser.sentencesWithFix());
  line += " | baud=";
  line += String(currentBaud);
#if GPS_AUTOBAUD_ENABLED
  line += " | ab=";
  line += (autobaudLocked ? String("lock") : String("scan@") + String(currentBaud));
#endif
  Serial.println(line);
}

// =================== API pública ===================
void gpsLogicBegin() {
  parser = TinyGPSPlus();
  serialReady = false;
  gpsStatus = GPS_STATUS_SEARCHING;
  gpsData.hasValidFix = false;
  gpsData.latitude = DEFAULT_LATITUDE;
  gpsData.longitude = DEFAULT_LONGITUDE;
  gpsData.timestamp = millis() / 1000;
  gpsData.satellites = 0;

  ensureSerial();
  lastDebug = millis();
  
#if GPS_AUTOBAUD_ENABLED
  autobaudLocked = false;
  autobaudIndex = 0;
  autobaudStartMs = 0;
  nmeaSniffLen = 0;
#endif
}

void gpsLogicEnable() {
  ensureSerial();
  if (gpsStatus == GPS_STATUS_OFF) {
    gpsStatus = GPS_STATUS_SEARCHING;
  }
}

void gpsLogicDisable() {
  gpsStatus = GPS_STATUS_OFF;
  gpsData.hasValidFix = false;
}

void gpsLogicUpdate() {
  if (gpsStatus == GPS_STATUS_OFF) return;
  ensureSerial();

  Stream& s = gpsStream();
  size_t processed = 0;
  while (s.available() && processed < GPS_STREAM_BUDGET) {
    int c = s.read();
    if (c < 0) break;
    parser.encode((char)c);
#if GPS_AUTOBAUD_ENABLED
    if (!autobaudLocked) {
      if (nmeaSniffLen < NMEA_SNIFF_MAX) {
        nmeaSniffBuf[nmeaSniffLen++] = (char)c;
      } else {
        // Mantener ventana reciente
        memmove(nmeaSniffBuf, nmeaSniffBuf + NMEA_SNIFF_MAX / 2, NMEA_SNIFF_MAX / 2);
        nmeaSniffLen = NMEA_SNIFF_MAX / 2;
        nmeaSniffBuf[nmeaSniffLen++] = (char)c;
      }
    }
#endif
    processed++;
  }

#if GPS_AUTOBAUD_ENABLED
  // Autobaud: rotar baudios hasta detectar líneas NMEA plausibles (con *hh)
  if (!autobaudLocked) {
    unsigned long now = millis();
    if (autobaudStartMs == 0) {
      beginSerialAt(kBaudCandidates[autobaudIndex]);
      autobaudStartMs = now;
    }
    auto isHex = [](char ch) {
      return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
    };
    int linesFound = 0;
    for (size_t i = 0; i + 6 < nmeaSniffLen; ++i) {
      if (nmeaSniffBuf[i] == '$') {
        size_t maxj = i + 80;
        if (maxj > nmeaSniffLen) maxj = nmeaSniffLen;
        for (size_t j = i + 3; j + 2 < maxj; ++j) {
          if (nmeaSniffBuf[j] == '*' && isHex(nmeaSniffBuf[j + 1]) && isHex(nmeaSniffBuf[j + 2])) {
            linesFound++;
            i = j + 2;
            break;
          }
        }
        if (linesFound >= 2) break;
      }
    }
    if (linesFound >= 2) {
      autobaudLocked = true;
    } else if (now - autobaudStartMs >= GPS_AUTOBAUD_WINDOW_MS) {
      autobaudIndex = (autobaudIndex + 1) % kBaudCount;
      beginSerialAt(kBaudCandidates[autobaudIndex]);
      autobaudStartMs = now;
      nmeaSniffLen = 0;
    }
  }
#endif

  updateSnapshot();
  debugPrint();
}

void gpsLogicReset() {
  gpsLogicBegin();
}

void gpsLogicSetUpdateInterval(uint16_t intervalMs) {
  (void)intervalMs;
}
