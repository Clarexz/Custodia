/*
 * GPS.CPP - Implementación del Sistema GPS Simulado (FIXED VERSION)
 * 
 * Este archivo implementa toda la funcionalidad del sistema GPS,
 * incluyendo múltiples modos de simulación realista y preparación
 * para integración con módulos GPS reales.
 * 
 * FIX: Asegurar que hasValidFix siempre sea true para GPS simulado
 */

#include "gps.h"
#include <math.h>

/*
 * CONSTANTES MATEMÁTICAS Y DE CONVERSIÓN
 */
#define EARTH_RADIUS_KM 6371.0f      // Radio de la Tierra en kilómetros
#define PI 3.14159265359f            // Valor de PI
#define DEG_TO_RAD (PI/180.0f)       // Conversión grados a radianes
#define RAD_TO_DEG (180.0f/PI)       // Conversión radianes a grados
#define METERS_PER_DEGREE_LAT 111320.0f  // Metros por grado de latitud

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
 * 
 * Inicializa todas las variables con valores por defecto
 */
GPSManager::GPSManager() {
    // Inicializar datos GPS con valores por defecto (SIMPLIFICADO)
    currentData.latitude = DEFAULT_LATITUDE;
    currentData.longitude = DEFAULT_LONGITUDE;
    currentData.satellites = 0;
    currentData.hasValidFix = false;
    currentData.timestamp = 0;
    
    // Estado inicial
    status = GPS_STATUS_OFF;
    simMode = GPS_SIM_FIXED;
    
    // Variables de simulación
    lastUpdateTime = 0;
    updateInterval = DEFAULT_UPDATE_INTERVAL;
    
    // Posición inicial de simulación (Reynosa, Tamaulipas)
    simStartLat = DEFAULT_LATITUDE;
    simStartLon = DEFAULT_LONGITUDE;
    simDirection = 0.0f;       // Norte
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
    // Limpieza si es necesaria (futuro)
}

/*
 * INICIALIZACIÓN DEL SISTEMA GPS
 */
void GPSManager::begin() {
    begin(GPS_SIM_FIXED);  // Modo fijo por defecto
}

void GPSManager::begin(GPSSimulationMode mode) {
    Serial.println("[GPS] Inicializando sistema GPS simplificado...");
    
    // Configurar modo de simulación
    simMode = mode;
    
    // Establecer posición inicial
    currentData.latitude = simStartLat;
    currentData.longitude = simStartLon;
    
    // Inicializar timestamp
    currentData.timestamp = millis() / 1000;  // Unix timestamp aproximado
    
    // Estado inicial
    status = GPS_STATUS_SEARCHING;
    lastUpdateTime = millis();
    startTime = millis();
    
    Serial.println("[GPS] Sistema GPS inicializado en modo: " + String(mode));
    Serial.println("[GPS] Posición inicial: " + String(simStartLat, 6) + ", " + String(simStartLon, 6));
    
    // Simular tiempo de búsqueda de satélites
    delay(2000);
    
    // Activar GPS con fix inicial
    enable();
}

/*
 * ACTUALIZACIÓN PRINCIPAL DEL GPS
 * 
 * Este método debe llamarse continuamente en el loop principal
 */
void GPSManager::update() {
    unsigned long currentTime = millis();
    
    // Verificar si es tiempo de actualizar
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        
        // Actualizar timestamp
        currentData.timestamp = currentTime / 1000;
        
        // Actualizar simulación según el modo
        updateSimulation();
        
        // Agregar ruido realista a las coordenadas
        addGPSNoise();
        
        // Actualizar número de satélites
        updateSatelliteCount();
        
        // Incrementar contador de actualizaciones
        totalUpdates++;
        
        // Debug opcional (comentar en producción)
        // printCurrentData();
    }
}

/*
 * CONTROL DE ENCENDIDO/APAGADO
 */
void GPSManager::enable() {
    if (status == GPS_STATUS_OFF) {
        Serial.println("[GPS] Encendiendo GPS...");
        status = GPS_STATUS_SEARCHING;
        
        // Simular tiempo de adquisición de fix
        delay(1000);
        
        // ASEGURAR QUE SIEMPRE TENEMOS FIX PARA SIMULACIÓN
        status = GPS_STATUS_FIX_3D;
        currentData.hasValidFix = true;
        currentData.satellites = 8;  // Número típico de satélites
        
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
 * ACTUALIZACIÓN DE SIMULACIÓN PRINCIPAL
 */
void GPSManager::updateSimulation() {
    // NUEVO FIX: ASEGURAR QUE SIEMPRE TENEMOS FIX VÁLIDO PARA SIMULACIÓN
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
            // Este modo alterna entre señal y pérdida
            if (random(0, 100) < 2) {  // REDUCIDO: 2% probabilidad de pérdida (era 5%)
                simulateSignalLoss(random(5, 15));  // REDUCIDO: 5-15 segundos (era 5-30)
            }
            updateFixedPosition();  // Posición fija cuando hay señal
            break;
            
        default:
            updateFixedPosition();
            break;
    }
    
    // NUEVO FIX: VERIFICAR AL FINAL QUE SEGUIMOS CON FIX VÁLIDO
    if (!signalLossActive && !currentData.hasValidFix) {
        Serial.println("[GPS] FIX: Re-estableciendo fix válido al final de update");
        currentData.hasValidFix = true;
        status = GPS_STATUS_FIX_3D;
    }
}

/*
 * MODOS ESPECÍFICOS DE SIMULACIÓN
 */

void GPSManager::updateFixedPosition() {
    // Posición fija - solo pequeñas variaciones por ruido
    currentData.latitude = simStartLat;
    currentData.longitude = simStartLon;
}

void GPSManager::updateLinearMovement() {
    // Movimiento en línea recta (SIMPLIFICADO)
    float distanceKm = (simSpeed * updateInterval) / (1000.0f * 3600.0f);  // Distancia en km
    
    // Convertir a cambios en coordenadas
    float deltaLat = (distanceKm * 1000.0f / METERS_PER_DEGREE_LAT) * cos(simDirection * DEG_TO_RAD);
    float deltaLon = (distanceKm * 1000.0f / (METERS_PER_DEGREE_LAT * cos(currentData.latitude * DEG_TO_RAD))) * sin(simDirection * DEG_TO_RAD);
    
    // Actualizar posición
    currentData.latitude += deltaLat;
    currentData.longitude += deltaLon;
    
    // Validar límites
    if (!isValidCoordinate(currentData.latitude, currentData.longitude)) {
        // Si salimos de límites, cambiar dirección
        simDirection = fmod(simDirection + 180.0f, 360.0f);
    }
}

void GPSManager::updateCircularMovement() {
    // Movimiento circular (patrullaje) - SIMPLIFICADO
    float radius = 0.01f;  // Radio en grados (aproximadamente 1km)
    float angularSpeed = (simSpeed / EARTH_RADIUS_KM) * RAD_TO_DEG;  // Velocidad angular
    
    // Incrementar ángulo
    float angle = (simStepCounter * angularSpeed * updateInterval / 1000.0f);
    
    // Calcular nueva posición
    currentData.latitude = simStartLat + radius * cos(angle * DEG_TO_RAD);
    currentData.longitude = simStartLon + radius * sin(angle * DEG_TO_RAD);
    
    simStepCounter++;
}

void GPSManager::updateRandomWalk() {
    // Caminata aleatoria (simula persona caminando) - SIMPLIFICADO
    float maxStepSize = 0.0001f;  // Paso máximo en grados (aprox 10 metros)
    
    // Cambios aleatorios pequeños
    float deltaLat = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    float deltaLon = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    
    // Aplicar cambios
    float newLat = currentData.latitude + deltaLat;
    float newLon = currentData.longitude + deltaLon;
    
    // Validar y aplicar
    if (isValidCoordinate(newLat, newLon)) {
        currentData.latitude = newLat;
        currentData.longitude = newLon;
    }
}

void GPSManager::updateSignalLoss() {
    unsigned long currentTime = millis();
    
    if (currentTime - signalLossStart >= (signalLossDuration * 1000)) {
        // Recuperar señal
        signalLossActive = false;
        status = GPS_STATUS_FIX_3D;
        currentData.hasValidFix = true;
        currentData.satellites = 8;
        Serial.println("[GPS] Señal GPS recuperada");
    } else {
        // Mantener pérdida de señal
        status = GPS_STATUS_SEARCHING;
        currentData.hasValidFix = false;
        currentData.satellites = random(0, 3);  // Pocos satélites
    }
}

/*
 * UTILIDADES DE SIMULACIÓN
 */

void GPSManager::addGPSNoise() {
    if (!currentData.hasValidFix) return;
    
    // Agregar ruido realista (típicamente ±3 metros)
    float noiseLat = (random(-300, 301) / 100000.0f) / METERS_PER_DEGREE_LAT;
    float noiseLon = (random(-300, 301) / 100000.0f) / (METERS_PER_DEGREE_LAT * cos(currentData.latitude * DEG_TO_RAD));
    
    currentData.latitude += noiseLat;
    currentData.longitude += noiseLon;
}

void GPSManager::updateSatelliteCount() {
    if (status == GPS_STATUS_FIX_3D) {
        // Variar número de satélites realísticamente
        int change = random(-1, 2);  // -1, 0, o 1
        currentData.satellites = constrain(currentData.satellites + change, 4, 12);
    } else if (status == GPS_STATUS_SEARCHING) {
        currentData.satellites = random(0, 4);
    }
}

/*
 * CÁLCULOS GEOGRÁFICOS
 */

float GPSManager::calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    // Fórmula de Haversine para calcular distancia
    float dLat = (lat2 - lat1) * DEG_TO_RAD;
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    
    float a = sin(dLat/2) * sin(dLat/2) + 
              cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) * 
              sin(dLon/2) * sin(dLon/2);
    
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return EARTH_RADIUS_KM * c * 1000.0f;  // Retornar en metros
}

float GPSManager::calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    // Calcular rumbo entre dos puntos
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    float lat1Rad = lat1 * DEG_TO_RAD;
    float lat2Rad = lat2 * DEG_TO_RAD;
    
    float y = sin(dLon) * cos(lat2Rad);
    float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
    
    float bearing = atan2(y, x) * RAD_TO_DEG;
    return fmod(bearing + 360.0f, 360.0f);  // Normalizar a 0-360
}

/*
 * CONFIGURACIÓN DE SIMULACIÓN
 */

void GPSManager::setSimulationMode(GPSSimulationMode mode) {
    simMode = mode;
    simStepCounter = 0;  // Reiniciar contador
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
    simSpeed = constrain(speedKmh, 0.1f, 200.0f);  // Limitar velocidad razonable
    Serial.println("[GPS] Velocidad de simulación: " + String(simSpeed) + " km/h");
}

void GPSManager::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = constrain(intervalMs, 100, 10000);  // 100ms a 10s
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
 * GETTERS - OBTENCIÓN DE DATOS (SIMPLIFICADOS)
 */

GPSData GPSManager::getCurrentData() {
    return currentData;  // Retorna copia
}

GPSData* GPSManager::getCurrentDataPtr() {
    return &currentData;  // Retorna puntero para acceso directo
}

float GPSManager::getLatitude() { return currentData.latitude; }
float GPSManager::getLongitude() { return currentData.longitude; }
bool GPSManager::hasValidFix() { return currentData.hasValidFix; }
uint32_t GPSManager::getTimestamp() { return currentData.timestamp; }
uint8_t GPSManager::getSatelliteCount() { return currentData.satellites; }  // Solo para debug

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
 * FORMATEO DE DATOS
 */

String GPSManager::formatCoordinates() {
    return String(currentData.latitude, 6) + "," + String(currentData.longitude, 6);
}

String GPSManager::formatForTransmission() {
    // Formato para transmisión LoRa: "deviceID,lat,lon,timestamp"
    // Nota: deviceID se agregará desde config cuando se llame esta función
    return String(currentData.latitude, 6) + "," + 
           String(currentData.longitude, 6) + "," + 
           String(currentData.timestamp);
}

String GPSManager::latitudeToString(int precision) {
    return String(currentData.latitude, precision);
}

String GPSManager::longitudeToString(int precision) {
    return String(currentData.longitude, precision);
}

/*
 * VALIDACIÓN
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

float GPSManager::bearingTo(float lat, float lon) {
    return calculateBearing(currentData.latitude, currentData.longitude, lat, lon);
}

/*
 * DEBUG Y DIAGNÓSTICO
 */

void GPSManager::printCurrentData() {
    Serial.println("\n=== DATOS GPS ACTUALES (SIMPLIFICADO) ===");
    Serial.println("Estado: " + getStatusString());
    Serial.println("Fix válido: " + String(currentData.hasValidFix ? "SÍ" : "NO"));
    Serial.println("Latitud: " + String(currentData.latitude, 6));
    Serial.println("Longitud: " + String(currentData.longitude, 6));
    Serial.println("Timestamp: " + String(currentData.timestamp));
    Serial.println("Satélites: " + String(currentData.satellites) + " (debug)");
    Serial.println("=======================================");
}

void GPSManager::printStatus() {
    Serial.println("[GPS] Estado: " + getStatusString() + 
                   " | Satélites: " + String(currentData.satellites) + 
                   " | Fix: " + String(currentData.hasValidFix ? "SÍ" : "NO"));
}

void GPSManager::printSimulationInfo() {
    Serial.println("\n=== INFORMACIÓN DE SIMULACIÓN ===");
    Serial.println("Modo: " + String(simMode));
    Serial.println("Posición inicial: " + String(simStartLat, 6) + ", " + String(simStartLon, 6));
    Serial.println("Velocidad: " + String(simSpeed) + " km/h");
    Serial.println("Dirección: " + String(simDirection) + "°");
    Serial.println("Intervalo actualización: " + String(updateInterval) + " ms");
    Serial.println("Actualizaciones totales: " + String(totalUpdates));
    Serial.println("Tiempo funcionamiento: " + String((millis() - startTime) / 1000) + " segundos");
    Serial.println("================================");
}

uint32_t GPSManager::getTotalUpdates() {
    return totalUpdates;
}

unsigned long GPSManager::getUptimeSeconds() {
    return (millis() - startTime) / 1000;
}