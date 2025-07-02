/*
 * GPS_MANAGER.CPP - Implementación del Coordinador GPS
 * 
 * MODULARIZADO de gps.cpp - Solo coordinación principal
 * Simulación movida a gps_simulation.cpp
 * Utils movidos a gps_utils.cpp
 * Battery movido a battery_manager.cpp
 */

#include "gps_manager.h"
#include "gps_simulation.h"
#include "gps_utils.h"
#include "../battery/battery_manager.h"

/*
 * INSTANCIA GLOBAL
 */
GPSManager gpsManager;

/*
 * VARIABLES ESTÁTICAS PARA ESTADÍSTICAS
 */
static uint32_t totalUpdates = 0;
static unsigned long startTime = 0;

/*
 * CONSTRUCTOR
 */
GPSManager::GPSManager() {
    // Inicializar datos GPS con valores por defecto
    currentData.latitude = DEFAULT_LATITUDE;
    currentData.longitude = DEFAULT_LONGITUDE;
    currentData.satellites = 0;
    currentData.hasValidFix = false;
    currentData.timestamp = 0;
    
    // Estado inicial
    status = GPS_STATUS_OFF;
    simMode = GPS_SIM_FIXED;
    
    // Variables de control
    lastUpdateTime = 0;
    updateInterval = DEFAULT_UPDATE_INTERVAL;
    
    // Inicializar componentes modulares
    simulation = &gpsSimulation;
    utils = &gpsUtils;
}

/*
 * DESTRUCTOR
 */
GPSManager::~GPSManager() {
    // Limpieza si es necesaria
}

/*
 * INICIALIZACIÓN DEL SISTEMA GPS
 */
void GPSManager::begin() {
    begin(GPS_SIM_FIXED);
}

void GPSManager::begin(GPSSimulationMode mode) {
    //Serial.println("[GPS] Inicializando sistema GPS simplificado...");
    
    simMode = mode;
    
    // Establecer posición inicial
    currentData.latitude = DEFAULT_LATITUDE;
    currentData.longitude = DEFAULT_LONGITUDE;
    
    // Inicializar timestamp
    currentData.timestamp = millis() / 1000;
    
    // Estado inicial
    status = GPS_STATUS_SEARCHING;
    lastUpdateTime = millis();
    startTime = millis();
    
    // Inicializar simulación
    simulation->begin(mode);
    
    //Serial.println("[GPS] Sistema GPS inicializado en modo: " + String(mode));
    Serial.println("[GPS] Posición inicial: " + String(DEFAULT_LATITUDE, 6) + ", " + String(DEFAULT_LONGITUDE, 6));
    
    // Simular tiempo de búsqueda de satélites
    delay(2000);
    
    // Activar GPS con fix inicial
    enable();
}

/*
 * ACTUALIZACIÓN PRINCIPAL DEL GPS
 */
void GPSManager::update() {
    unsigned long currentTime = millis();
    
    // Verificar si es tiempo de actualizar
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        
        // Actualizar timestamp
        currentData.timestamp = currentTime / 1000;
        
        // Actualizar simulación
        currentData = simulation->updateSimulation(currentData, simMode);
        
        // Incrementar contador de actualizaciones
        totalUpdates++;
    }
}

/*
 * CONTROL DE ENCENDIDO/APAGADO
 */
void GPSManager::enable() {
    if (status == GPS_STATUS_OFF) {
        //Serial.println("[GPS] Encendiendo GPS...");
        status = GPS_STATUS_SEARCHING;
        
        delay(1000);
        
        status = GPS_STATUS_FIX_3D;
        currentData.hasValidFix = true;
        currentData.satellites = 8;
        
        //Serial.println("[GPS] GPS activado - Fix obtenido");
        //Serial.println("[GPS] VALIDFIX: " + String(currentData.hasValidFix));
    }
}

void GPSManager::disable() {
    Serial.println("[GPS] Apagando GPS para ahorrar energía...");
    status = GPS_STATUS_OFF;
    currentData.hasValidFix = false;
    currentData.satellites = 0;
}

void GPSManager::reset() {
    Serial.println("[GPS] Reiniciando GPS...");
    disable();
    delay(500);
    enable();
}

/*
 * GETTERS - OBTENCIÓN DE DATOS
 */
GPSData GPSManager::getCurrentData() {
    return currentData;
}

GPSData* GPSManager::getCurrentDataPtr() {
    return &currentData;
}

float GPSManager::getLatitude() { return currentData.latitude; }
float GPSManager::getLongitude() { return currentData.longitude; }
bool GPSManager::hasValidFix() { return currentData.hasValidFix; }
uint32_t GPSManager::getTimestamp() { return currentData.timestamp; }
uint8_t GPSManager::getSatelliteCount() { return currentData.satellites; }

GPSStatus GPSManager::getStatus() { return status; }

String GPSManager::getStatusString() {
    switch (status) {
        case GPS_STATUS_OFF: return "APAGADO";
        case GPS_STATUS_SEARCHING: return "BUSCANDO";
        case GPS_STATUS_FIX_2D: return "FIX 2D";
        case GPS_STATUS_FIX_3D: return "FIX 3D";
        case GPS_STATUS_ERROR: return "ERROR";
        default: return "DESCONOCIDO";
    }
}

bool GPSManager::isEnabled() {
    return status != GPS_STATUS_OFF;
}

/*
 * FORMATEO DE DATOS - ACTUALIZADO para usar batteryManager
 */
String GPSManager::formatCoordinates() {
    return String(currentData.latitude, 6) + "," + String(currentData.longitude, 6);
}

String GPSManager::formatForTransmission() {
    return String(currentData.latitude, 6) + "," + 
           String(currentData.longitude, 6) + "," + 
           String(currentData.timestamp);
}

String GPSManager::formatPacketWithDeviceID(uint16_t deviceID) {
    // Formato exacto según key requirements: [deviceID, latitude, longitude, batteryvoltage, timestamp]
    return String(deviceID) + "," +
           String(currentData.latitude, 6) + "," + 
           String(currentData.longitude, 6) + "," + 
           String(batteryManager.getVoltage()) + "," +
           String(currentData.timestamp);
}

String GPSManager::latitudeToString(int precision) {
    return String(currentData.latitude, precision);
}

String GPSManager::longitudeToString(int precision) {
    return String(currentData.longitude, precision);
}

/*
 * CONFIGURACIÓN DE SIMULACIÓN - Delegada a simulation
 */
void GPSManager::setSimulationMode(GPSSimulationMode mode) {
    simMode = mode;
    simulation->setSimulationMode(mode);
}

void GPSManager::setStartPosition(float lat, float lon) {
    simulation->setStartPosition(lat, lon);
}

void GPSManager::setSimulationSpeed(float speedKmh) {
    simulation->setSimulationSpeed(speedKmh);
}

void GPSManager::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = constrain(intervalMs, 100, 10000);
    Serial.println("[GPS] Intervalo de actualización: " + String(updateInterval) + " ms");
}

void GPSManager::setSimulationDirection(float degrees) {
    simulation->setSimulationDirection(degrees);
}

void GPSManager::simulateSignalLoss(uint16_t durationSeconds) {
    simulation->simulateSignalLoss(durationSeconds);
}

/*
 * VALIDACIÓN - Delegada a utils
 */
bool GPSManager::isValidLatitude(float lat) {
    return utils->isValidLatitude(lat);
}

bool GPSManager::isValidLongitude(float lon) {
    return utils->isValidLongitude(lon);
}

bool GPSManager::isValidCoordinate(float lat, float lon) {
    return utils->isValidCoordinate(lat, lon);
}

float GPSManager::distanceTo(float lat, float lon) {
    return utils->calculateDistance(currentData.latitude, currentData.longitude, lat, lon);
}

float GPSManager::bearingTo(float lat, float lon) {
    return utils->calculateBearing(currentData.latitude, currentData.longitude, lat, lon);
}

/*
 * DEBUG Y DIAGNÓSTICO
 */
void GPSManager::printCurrentData() {
    Serial.println("\n=== DATOS GPS ACTUALES ===");
    Serial.println("Estado: " + getStatusString());
    Serial.println("Fix válido: " + String(currentData.hasValidFix ? "SÍ" : "NO"));
    Serial.println("Latitud: " + String(currentData.latitude, 6));
    Serial.println("Longitud: " + String(currentData.longitude, 6));
    Serial.println("Timestamp: " + String(currentData.timestamp));
    Serial.println("Satélites: " + String(currentData.satellites));
    Serial.println("==============================");
}

void GPSManager::printStatus() {
    Serial.println("[GPS] Estado: " + getStatusString() + 
                   " | Satélites: " + String(currentData.satellites) + 
                   " | Fix: " + String(currentData.hasValidFix ? "SÍ" : "NO"));
}

void GPSManager::printSimulationInfo() {
    Serial.println("\n=== INFORMACIÓN DE SIMULACIÓN ===");
    Serial.println("Modo: " + String(simMode));
    Serial.println("Intervalo actualización: " + String(updateInterval) + " ms");
    Serial.println("Actualizaciones totales: " + String(totalUpdates));
    Serial.println("Tiempo funcionamiento: " + String(getUptimeSeconds()) + " segundos");
    Serial.println("=================================");
}

uint32_t GPSManager::getTotalUpdates() {
    return totalUpdates;
}

unsigned long GPSManager::getUptimeSeconds() {
    return (millis() - startTime) / 1000;
}