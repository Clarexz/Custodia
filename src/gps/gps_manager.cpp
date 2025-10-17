#include "gps_manager.h"
#include "gps_logic.h"
#include "../battery/battery_manager.h"

GPSManager gpsManager;

GPSManager::GPSManager() {
    updateInterval = DEFAULT_UPDATE_INTERVAL;
    totalUpdates = 0;
    startTime = millis();
    // Importante: no inicializar hardware aquí (antes de setup()).
    // La inicialización real se hace en begin(), invocada por RoleManager
    // cuando el sistema ya está listo.
}

void GPSManager::begin() {
    totalUpdates = 0;
    startTime = millis();
    gpsLogicSetUpdateInterval(updateInterval);
    gpsLogicBegin();
}

void GPSManager::update() {
    gpsLogicUpdate();
    totalUpdates++;
}

void GPSManager::enable() {
    gpsLogicEnable();
}

void GPSManager::disable() {
    gpsLogicDisable();
}

void GPSManager::reset() {
    gpsLogicReset();
}

void GPSManager::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = constrain(intervalMs, 100, 10000);
    gpsLogicSetUpdateInterval(updateInterval);
}

GPSData GPSManager::getCurrentData() {
    return gpsData;
}

GPSData* GPSManager::getCurrentDataPtr() {
    return &gpsData;
}

float GPSManager::getLatitude() { return gpsData.latitude; }
float GPSManager::getLongitude() { return gpsData.longitude; }
bool GPSManager::hasValidFix() { return gpsData.hasValidFix; }
uint32_t GPSManager::getTimestamp() { return gpsData.timestamp; }
uint8_t GPSManager::getSatelliteCount() { return gpsData.satellites; }

GPSStatus GPSManager::getStatus() { return gpsStatus; }

String GPSManager::getStatusString() {
    switch (gpsStatus) {
        case GPS_STATUS_OFF: return "APAGADO";
        case GPS_STATUS_SEARCHING: return "BUSCANDO";
        case GPS_STATUS_FIX_2D: return "FIX 2D";
        case GPS_STATUS_FIX_3D: return "FIX 3D";
        case GPS_STATUS_ERROR: return "ERROR";
        default: return "DESCONOCIDO";
    }
}

bool GPSManager::isEnabled() {
    return gpsStatus != GPS_STATUS_OFF;
}

String GPSManager::formatCoordinates() {
    return String(gpsData.latitude, 6) + "," + String(gpsData.longitude, 6);
}

String GPSManager::formatForTransmission() {
    return String(gpsData.latitude, 6) + "," + String(gpsData.longitude, 6) + "," + String(gpsData.timestamp);
}

String GPSManager::formatPacketWithDeviceID(uint16_t deviceID) {
    return String(deviceID) + "," +
           String(gpsData.latitude, 6) + "," +
           String(gpsData.longitude, 6) + "," +
           String(batteryManager.getVoltage()) + "," +
           String(gpsData.timestamp);
}

String GPSManager::latitudeToString(int precision) {
    return String(gpsData.latitude, precision);
}

String GPSManager::longitudeToString(int precision) {
    return String(gpsData.longitude, precision);
}

bool GPSManager::isValidLatitude(float lat) {
    return (lat >= MIN_LATITUDE && lat <= MAX_LATITUDE);
}

bool GPSManager::isValidLongitude(float lon) {
    return (lon >= MIN_LONGITUDE && lon <= MAX_LONGITUDE);
}

bool GPSManager::isValidCoordinate(float lat, float lon) {
    return isValidLatitude(lat) && isValidLongitude(lon);
}

float GPSManager::distanceTo(float lat, float lon) {
    (void)lat;
    (void)lon;
    return 0.0f;
}

float GPSManager::bearingTo(float lat, float lon) {
    (void)lat;
    (void)lon;
    return 0.0f;
}

void GPSManager::printCurrentData() {
    Serial.println("\n=== DATOS GPS ACTUALES ===");
    Serial.println("Estado: " + getStatusString());
    Serial.println("Fix válido: " + String(gpsData.hasValidFix ? "SÍ" : "NO"));
    Serial.println("Latitud: " + String(gpsData.latitude, 6));
    Serial.println("Longitud: " + String(gpsData.longitude, 6));
    Serial.println("Timestamp: " + String(gpsData.timestamp));
    Serial.println("Satélites: " + String(gpsData.satellites));
    Serial.println("==============================");
}

void GPSManager::printStatus() {
    Serial.println("[GPS] Estado: " + getStatusString() +
                   " | Satélites: " + String(gpsData.satellites) +
                   " | Fix: " + String(gpsData.hasValidFix ? "SÍ" : "NO"));
}

void GPSManager::printSimulationInfo() {
    Serial.println("\n=== INFORMACIÓN GPS ===");
    Serial.println("Intervalo actualización: " + String(updateInterval) + " ms");
    Serial.println("Actualizaciones totales: " + String(totalUpdates));
    Serial.println("Tiempo funcionamiento: " + String(getUptimeSeconds()) + " s");
    Serial.println("=================================");
}

uint32_t GPSManager::getTotalUpdates() {
    return totalUpdates;
}

unsigned long GPSManager::getUptimeSeconds() {
    return (millis() - startTime) / 1000;
}
