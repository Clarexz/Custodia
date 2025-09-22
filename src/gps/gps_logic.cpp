#include <Arduino.h>
#include "gps_logic.h"

/* ============================================================================
 * GPS LOGIC (MINIMAL)
 * ---------------------------------------------------------------------------
 * - Mantén las variables globales y funciones públicas tal cual están.
 * - Por ahora publicamos dos contadores sencillos (1 en 1 y 2 en 2) para
 *   simular latitud/longitud.
 * - Cuando integres tu GPS real, sustituye el contenido de las funciones por
 *   las lecturas reales y asigna los resultados a `gpsData` / `gpsStatus`.
 * ============================================================================
 */
/* ============================================================================
 * NOTAS PARA INTEGRAR HARDWARE REAL
 * ---------------------------------------------------------------------------
 * 1. Reemplaza el cuerpo de gpsLogicUpdate (y, si quieres, de las demás
 *    funciones) con tu lectura real. Asigna latitud/longitud al struct `gpsData`
 *    y actualiza `gpsStatus`/`gpsData.hasValidFix` según corresponda.
 * 2. Los contadores `counterLat`/`counterLon` son meramente un placeholder; puedes
 *    eliminarlos cuando uses el GPS real.
 * 3. `gpsLogicSetUpdateInterval` ajusta la cadencia de lecturas; respeta esa
 *    configuración si tu módulo lo permite.
 * ============================================================================
 */


// Variables finales que el resto del firmware consume.
// Sustituye estas dos por la lectura real de tu módulo cuando lo integres.
float finalLatitude = DEFAULT_LATITUDE;
float finalLongitude = DEFAULT_LONGITUDE;

// Datos GPS compartidos con el resto del firmware.
// Campos: latitude, longitude, hasValidFix, timestamp, satellites.
GPSData gpsData = {DEFAULT_LATITUDE, DEFAULT_LONGITUDE, false, 0, 0};
// Estado actual (OFF/FIX/...) que consume GPSManager.
GPSStatus gpsStatus = GPS_STATUS_OFF;

// Contadores placeholder que imitan latitud/longitud incrementales.
static uint16_t counterLat = 0;
static uint16_t counterLon = 0;


void gpsLogicBegin() {
    gpsStatus = GPS_STATUS_FIX_3D;
    gpsData.hasValidFix = true;
    gpsData.satellites = 0;
    gpsData.timestamp = millis() / 1000;
    counterLat = 0;
    counterLon = 0;
    finalLatitude = 0.0f;
    finalLongitude = 0.0f;
    gpsData.latitude = finalLatitude;
    gpsData.longitude = finalLongitude;
}

void gpsLogicEnable() {
    gpsStatus = GPS_STATUS_FIX_3D;
    gpsData.hasValidFix = true;
}

void gpsLogicDisable() {
    gpsStatus = GPS_STATUS_OFF;
    gpsData.hasValidFix = false;
}

void gpsLogicUpdate() {
    if (gpsStatus == GPS_STATUS_OFF) {
        return;
    }

    gpsData.timestamp = millis() / 1000;
    counterLat += 1;
    counterLon += 2;
    finalLatitude = static_cast<float>(counterLat);
    
    finalLongitude = static_cast<float>(counterLon);
    gpsData.latitude  = finalLatitude;
    gpsData.longitude = finalLongitude;
    gpsData.hasValidFix = true;
}

void gpsLogicReset() {
    gpsLogicBegin();
}

void gpsLogicSetUpdateInterval(uint16_t intervalMs) {
    (void)intervalMs;
}
