/*
 * GPS.CPP - Implementación del Sistema GPS Simulado + Battery Monitoring
 * 
 * ACTUALIZACIÓN: Agregando simulación de battery voltage para cumplir
 * con el packet format requerido: [deviceID, lat, lon, batteryvoltage, timestamp]
 */

#include "gps.h"
#include <math.h>

/*
 * CONSTANTES MATEMÁTICAS Y DE CONVERSIÓN
 */
#define EARTH_RADIUS_KM 6371.0f
#define PI 3.14159265359f
#define DEG_TO_RAD (PI/180.0f)
#define RAD_TO_DEG (180.0f/PI)
#define METERS_PER_DEGREE_LAT 111320.0f

// NUEVO: Constantes para simulación de batería
#define BATTERY_MAX_VOLTAGE 4200  // mV - Batería completamente cargada (4.2V)
#define BATTERY_MIN_VOLTAGE 3200  // mV - Batería descargada (3.2V)
#define BATTERY_DRAIN_RATE 0.1f   // mV por minuto de simulación

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
 * CONSTRUCTOR - ACTUALIZADO para incluir battery
 */
GPSManager::GPSManager() {
    // Inicializar datos GPS con valores por defecto
    currentData.latitude = DEFAULT_LATITUDE;
    currentData.longitude = DEFAULT_LONGITUDE;
    currentData.satellites = 0;
    currentData.hasValidFix = false;
    currentData.timestamp = 0;
    
    // NUEVO: Inicializar battery voltage (batería llena)
    currentData.batteryVoltage = BATTERY_MAX_VOLTAGE;
    
    // Estado inicial
    status = GPS_STATUS_OFF;
    simMode = GPS_SIM_FIXED;
    
    // Variables de simulación
    lastUpdateTime = 0;
    updateInterval = DEFAULT_UPDATE_INTERVAL;
    
    // Posición inicial de simulación (Reynosa, Tamaulipas)
    simStartLat = DEFAULT_LATITUDE;
    simStartLon = DEFAULT_LONGITUDE;
    simDirection = 0.0f;
    simSpeed = DEFAULT_SIM_SPEED;
    simStepCounter = 0;
    
    // Simulación de pérdida de señal
    signalLossActive = false;
    signalLossStart = 0;
    signalLossDuration = 0;
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
    Serial.println("[GPS] Inicializando sistema GPS simplificado...");
    
    simMode = mode;
    
    // Establecer posición inicial
    currentData.latitude = simStartLat;
    currentData.longitude = simStartLon;
    
    // Inicializar timestamp
    currentData.timestamp = millis() / 1000;
    
    // NUEVO: Inicializar battery voltage con variación aleatoria
    currentData.batteryVoltage = BATTERY_MAX_VOLTAGE - random(0, 200); // 4.0-4.2V
    
    // Estado inicial
    status = GPS_STATUS_SEARCHING;
    lastUpdateTime = millis();
    startTime = millis();
    
    Serial.println("[GPS] Sistema GPS inicializado en modo: " + String(mode));
    Serial.println("[GPS] Posición inicial: " + String(simStartLat, 6) + ", " + String(simStartLon, 6));
    Serial.println("[GPS] Battery inicial: " + String(currentData.batteryVoltage) + " mV");
    
    // Simular tiempo de búsqueda de satélites
    delay(2000);
    
    // Activar GPS con fix inicial
    enable();
}

/*
 * ACTUALIZACIÓN PRINCIPAL DEL GPS - ACTUALIZADA para incluir battery
 */
void GPSManager::update() {
    unsigned long currentTime = millis();
    
    // Verificar si es tiempo de actualizar
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        
        // Actualizar timestamp
        currentData.timestamp = currentTime / 1000;
        
        // NUEVO: Actualizar battery voltage (simular descarga gradual)
        updateBatteryVoltage();
        
        // Actualizar simulación según el modo
        updateSimulation();
        
        // Agregar ruido realista a las coordenadas
        addGPSNoise();
        
        // Actualizar número de satélites
        updateSatelliteCount();
        
        // Incrementar contador de actualizaciones
        totalUpdates++;
    }
}

/*
 * NUEVO: MÉTODO PARA ACTUALIZAR BATTERY VOLTAGE
 */
void GPSManager::updateBatteryVoltage() {
    // Simular descarga gradual de batería
    unsigned long runningTime = millis() - startTime;
    float minutesRunning = runningTime / 60000.0f;
    
    // Calcular voltage basado en tiempo transcurrido
    float drainedVoltage = minutesRunning * BATTERY_DRAIN_RATE;
    currentData.batteryVoltage = BATTERY_MAX_VOLTAGE - (uint16_t)drainedVoltage;
    
    // Asegurar que no baje del mínimo
    if (currentData.batteryVoltage < BATTERY_MIN_VOLTAGE) {
        currentData.batteryVoltage = BATTERY_MIN_VOLTAGE;
    }
    
    // Agregar pequeña variación aleatoria (±10mV)
    int variation = random(-10, 11);
    currentData.batteryVoltage = constrain(currentData.batteryVoltage + variation, 
                                         BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
}

/*
 * CONTROL DE ENCENDIDO/APAGADO (sin cambios)
 */
void GPSManager::enable() {
    if (status == GPS_STATUS_OFF) {
        Serial.println("[GPS] Encendiendo GPS...");
        status = GPS_STATUS_SEARCHING;
        
        delay(1000);
        
        status = GPS_STATUS_FIX_3D;
        currentData.hasValidFix = true;
        currentData.satellites = 8;
        
        Serial.println("[GPS] GPS activado - Fix obtenido");
        Serial.println("[GPS] VALIDFIX: " + String(currentData.hasValidFix));
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
 * ACTUALIZACIÓN DE SIMULACIÓN PRINCIPAL (sin cambios importantes)
 */
void GPSManager::updateSimulation() {
    // Asegurar que siempre tenemos fix válido para simulación
    if (!currentData.hasValidFix && !signalLossActive) {
        Serial.println("[GPS] FIX: Forzando fix válido para simulación");
        currentData.hasValidFix = true;
        status = GPS_STATUS_FIX_3D;
        currentData.satellites = 8;
    }
    
    // Si hay pérdida de señal activa, manejarla primero
    if (signalLossActive) {
        updateSignalLoss();
        return;
    }
    
    // Ejecutar simulación según el modo configurado
    switch (simMode) {
        case GPS_SIM_FIXED:
            updateFixedPosition();
            break;
            
        case GPS_SIM_LINEAR:
            updateLinearMovement();
            break;
            
        case GPS_SIM_CIRCULAR:
            updateCircularMovement();
            break;
            
        case GPS_SIM_RANDOM_WALK:
            updateRandomWalk();
            break;
            
        case GPS_SIM_SIGNAL_LOSS:
            if (random(0, 100) < 2) {
                simulateSignalLoss(random(5, 15));
            }
            updateFixedPosition();
            break;
            
        default:
            updateFixedPosition();
            break;
    }
    
    // Verificar al final que seguimos con fix válido
    if (!signalLossActive && !currentData.hasValidFix) {
        Serial.println("[GPS] FIX: Re-estableciendo fix válido al final de update");
        currentData.hasValidFix = true;
        status = GPS_STATUS_FIX_3D;
    }
}

/*
 * MODOS ESPECÍFICOS DE SIMULACIÓN (sin cambios)
 */

void GPSManager::updateFixedPosition() {
    currentData.latitude = simStartLat;
    currentData.longitude = simStartLon;
}

void GPSManager::updateLinearMovement() {
    float distanceKm = (simSpeed * updateInterval) / (1000.0f * 3600.0f);
    
    float deltaLat = (distanceKm * 1000.0f / METERS_PER_DEGREE_LAT) * cos(simDirection * DEG_TO_RAD);
    float deltaLon = (distanceKm * 1000.0f / (METERS_PER_DEGREE_LAT * cos(currentData.latitude * DEG_TO_RAD))) * sin(simDirection * DEG_TO_RAD);
    
    currentData.latitude += deltaLat;
    currentData.longitude += deltaLon;
    
    if (!isValidCoordinate(currentData.latitude, currentData.longitude)) {
        simDirection = fmod(simDirection + 180.0f, 360.0f);
    }
}

void GPSManager::updateCircularMovement() {
    float radius = 0.01f;
    float angularSpeed = (simSpeed / EARTH_RADIUS_KM) * RAD_TO_DEG;
    
    float angle = (simStepCounter * angularSpeed * updateInterval / 1000.0f);
    
    currentData.latitude = simStartLat + radius * cos(angle * DEG_TO_RAD);
    currentData.longitude = simStartLon + radius * sin(angle * DEG_TO_RAD);
    
    simStepCounter++;
}

void GPSManager::updateRandomWalk() {
    float maxStepSize = 0.0001f;
    
    float deltaLat = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    float deltaLon = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    
    float newLat = currentData.latitude + deltaLat;
    float newLon = currentData.longitude + deltaLon;
    
    if (isValidCoordinate(newLat, newLon)) {
        currentData.latitude = newLat;
        currentData.longitude = newLon;
    }
}

void GPSManager::updateSignalLoss() {
    unsigned long currentTime = millis();
    
    if (currentTime - signalLossStart >= (signalLossDuration * 1000)) {
        signalLossActive = false;
        status = GPS_STATUS_FIX_3D;
        currentData.hasValidFix = true;
        currentData.satellites = 8;
        Serial.println("[GPS] Señal GPS recuperada");
    } else {
        status = GPS_STATUS_SEARCHING;
        currentData.hasValidFix = false;
        currentData.satellites = random(0, 3);
    }
}

/*
 * UTILIDADES DE SIMULACIÓN (sin cambios)
 */

void GPSManager::addGPSNoise() {
    if (!currentData.hasValidFix) return;
    
    float noiseLat = (random(-300, 301) / 100000.0f) / METERS_PER_DEGREE_LAT;
    float noiseLon = (random(-300, 301) / 100000.0f) / (METERS_PER_DEGREE_LAT * cos(currentData.latitude * DEG_TO_RAD));
    
    currentData.latitude += noiseLat;
    currentData.longitude += noiseLon;
}

void GPSManager::updateSatelliteCount() {
    if (status == GPS_STATUS_FIX_3D) {
        int change = random(-1, 2);
        currentData.satellites = constrain(currentData.satellites + change, 4, 12);
    } else if (status == GPS_STATUS_SEARCHING) {
        currentData.satellites = random(0, 4);
    }
}

/*
 * CÁLCULOS GEOGRÁFICOS (sin cambios)
 */

float GPSManager::calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * DEG_TO_RAD;
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    
    float a = sin(dLat/2) * sin(dLat/2) + 
              cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) * 
              sin(dLon/2) * sin(dLon/2);
    
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return EARTH_RADIUS_KM * c * 1000.0f;
}

float GPSManager::bearingTo(float lat, float lon) {
    return calculateBearing(currentData.latitude, currentData.longitude, lat, lon);
}

float GPSManager::calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    float lat1Rad = lat1 * DEG_TO_RAD;
    float lat2Rad = lat2 * DEG_TO_RAD;
    
    float y = sin(dLon) * cos(lat2Rad);
    float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
    
    float bearing = atan2(y, x) * RAD_TO_DEG;
    return fmod(bearing + 360.0f, 360.0f);
}

/*
 * CONFIGURACIÓN DE SIMULACIÓN (sin cambios)
 */

void GPSManager::setSimulationMode(GPSSimulationMode mode) {
    simMode = mode;
    simStepCounter = 0;
    Serial.println("[GPS] Modo de simulación cambiado a: " + String(mode));
}

void GPSManager::setStartPosition(float lat, float lon) {
    if (isValidCoordinate(lat, lon)) {
        simStartLat = lat;
        simStartLon = lon;
        currentData.latitude = lat;
        currentData.longitude = lon;
        Serial.println("[GPS] Posición inicial establecida: " + String(lat, 6) + ", " + String(lon, 6));
    } else {
        Serial.println("[GPS] ERROR: Coordenadas inválidas");
    }
}

void GPSManager::setSimulationSpeed(float speedKmh) {
    simSpeed = constrain(speedKmh, 0.1f, 200.0f);
    Serial.println("[GPS] Velocidad de simulación: " + String(simSpeed) + " km/h");
}

void GPSManager::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = constrain(intervalMs, 100, 10000);
    Serial.println("[GPS] Intervalo de actualización: " + String(updateInterval) + " ms");
}

void GPSManager::setSimulationDirection(float degrees) {
    simDirection = fmod(degrees, 360.0f);
    if (simDirection < 0) simDirection += 360.0f;
    Serial.println("[GPS] Dirección de simulación: " + String(simDirection) + "°");
}

void GPSManager::simulateSignalLoss(uint16_t durationSeconds) {
    signalLossActive = true;
    signalLossStart = millis();
    signalLossDuration = durationSeconds;
    status = GPS_STATUS_SEARCHING;
    currentData.hasValidFix = false;
    Serial.println("[GPS] Simulando pérdida de señal por " + String(durationSeconds) + " segundos");
}

/*
 * GETTERS - OBTENCIÓN DE DATOS - ACTUALIZADOS para incluir battery
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

// NUEVO: Getter para battery voltage
uint16_t GPSManager::getBatteryVoltage() { return currentData.batteryVoltage; }

// NUEVO: Getter para battery percentage
uint8_t GPSManager::getBatteryPercentage() {
    if (currentData.batteryVoltage <= BATTERY_MIN_VOLTAGE) return 0;
    if (currentData.batteryVoltage >= BATTERY_MAX_VOLTAGE) return 100;
    
    return ((currentData.batteryVoltage - BATTERY_MIN_VOLTAGE) * 100) / 
           (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);
}

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
 * FORMATEO DE DATOS - ACTUALIZADO para incluir battery
 */

String GPSManager::formatCoordinates() {
    return String(currentData.latitude, 6) + "," + String(currentData.longitude, 6);
}

String GPSManager::formatForTransmission() {
    // NUEVO: Formato para transmisión LoRa según key requirements:
    // "deviceID,lat,lon,batteryvoltage,timestamp"
    // Nota: deviceID se agregará desde config cuando se llame esta función
    return String(currentData.latitude, 6) + "," + 
           String(currentData.longitude, 6) + "," + 
           String(currentData.batteryVoltage) + "," +
           String(currentData.timestamp);
}

// NUEVO: Formatear packet completo con deviceID
String GPSManager::formatPacketWithDeviceID(uint16_t deviceID) {
    // Formato exacto según key requirements: [deviceID, latitude, longitude, batteryvoltage, timestamp]
    return String(deviceID) + "," +
           String(currentData.latitude, 6) + "," + 
           String(currentData.longitude, 6) + "," + 
           String(currentData.batteryVoltage) + "," +
           String(currentData.timestamp);
}

String GPSManager::latitudeToString(int precision) {
    return String(currentData.latitude, precision);
}

String GPSManager::longitudeToString(int precision) {
    return String(currentData.longitude, precision);
}

/*
 * VALIDACIÓN (sin cambios)
 */

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
    return calculateDistance(currentData.latitude, currentData.longitude, lat, lon);
}

/*
 * DEBUG Y DIAGNÓSTICO - ACTUALIZADO para incluir battery
 */

void GPSManager::printCurrentData() {
    Serial.println("\n=== DATOS GPS ACTUALES ===");
    Serial.println("Estado: " + getStatusString());
    Serial.println("Fix válido: " + String(currentData.hasValidFix ? "SÍ" : "NO"));
    Serial.println("Latitud: " + String(currentData.latitude, 6));
    Serial.println("Longitud: " + String(currentData.longitude, 6));
    Serial.println("Battery: " + String(currentData.batteryVoltage) + " mV (" + String(getBatteryPercentage()) + "%)");
    Serial.println("Timestamp: " + String(currentData.timestamp));
    Serial.println("Satélites: " + String(currentData.satellites));
    Serial.println("==============================");
}

void GPSManager::printStatus() {
    Serial.println("[GPS] Estado: " + getStatusString() + 
                   " | Satélites: " + String(currentData.satellites) + 
                   " | Fix: " + String(currentData.hasValidFix ? "SÍ" : "NO") +
                   " | Battery: " + String(currentData.batteryVoltage) + "mV");
}

void GPSManager::printSimulationInfo() {
    Serial.println("\n=== INFORMACIÓN DE SIMULACIÓN ===");
    Serial.println("Modo: " + String(simMode));
    Serial.println("Posición inicial: " + String(simStartLat, 6) + ", " + String(simStartLon, 6));
    Serial.println("Velocidad: " + String(simSpeed) + " km/h");
    Serial.println("Dirección: " + String(simDirection) + "°");
    Serial.println("Intervalo actualización: " + String(updateInterval) + " ms");
    Serial.println("Battery actual: " + String(currentData.batteryVoltage) + " mV");
    Serial.println("Actualizaciones totales: " + String(totalUpdates));
    Serial.println("Tiempo funcionamiento: " + String((millis() - startTime) / 1000) + " segundos");
    Serial.println("=================================");
}

uint32_t GPSManager::getTotalUpdates() {
    return totalUpdates;
}

unsigned long GPSManager::getUptimeSeconds() {
    return (millis() - startTime) / 1000;
}