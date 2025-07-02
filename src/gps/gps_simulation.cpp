/*
 * GPS_SIMULATION.CPP - Implementación de Todos los Modos de Simulación GPS
 * 
 * EXTRAÍDO de gps.cpp - Todos los métodos update*Movement()
 */

#include "gps_simulation.h"
#include "gps_utils.h"
#include <math.h>

/*
 * INSTANCIA GLOBAL
 */
GPSSimulation gpsSimulation;

/*
 * CONSTRUCTOR
 */
GPSSimulation::GPSSimulation() {
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
GPSSimulation::~GPSSimulation() {
    // Limpieza si es necesaria
}

/*
 * INICIALIZACIÓN
 */
void GPSSimulation::begin(GPSSimulationMode mode) {
    Serial.println("[GPS Simulation] Inicializado en modo: " + String(mode));
}

/*
 * ACTUALIZACIÓN PRINCIPAL DE SIMULACIÓN - EXTRAÍDO de updateSimulation()
 */
GPSData GPSSimulation::updateSimulation(GPSData currentData, GPSSimulationMode mode) {
    GPSData data = currentData;
    
    // Asegurar que siempre tenemos fix válido para simulación
    if (!data.hasValidFix && !signalLossActive) {
        Serial.println("[GPS] FIX: Forzando fix válido para simulación");
        data.hasValidFix = true;
        data.satellites = 8;
    }
    
    // Si hay pérdida de señal activa, manejarla primero
    if (signalLossActive) {
        updateSignalLoss(data);
        return data;
    }
    
    // Ejecutar simulación según el modo configurado
    switch (mode) {
        case GPS_SIM_FIXED:
            updateFixedPosition(data);
            break;
            
        case GPS_SIM_LINEAR:
            updateLinearMovement(data);
            break;
            
        case GPS_SIM_CIRCULAR:
            updateCircularMovement(data);
            break;
            
        case GPS_SIM_RANDOM_WALK:
            updateRandomWalk(data);
            break;
            
        case GPS_SIM_SIGNAL_LOSS:
            if (random(0, 100) < 2) {
                simulateSignalLoss(random(5, 15));
            }
            updateFixedPosition(data);
            break;
            
        default:
            updateFixedPosition(data);
            break;
    }
    
    // Agregar ruido realista a las coordenadas
    addGPSNoise(data);
    
    // Actualizar número de satélites
    updateSatelliteCount(data, GPS_STATUS_FIX_3D);
    
    // Verificar al final que seguimos con fix válido
    if (!signalLossActive && !data.hasValidFix) {
        Serial.println("[GPS] FIX: Re-estableciendo fix válido al final de update");
        data.hasValidFix = true;
    }
    
    return data;
}

/*
 * MODOS ESPECÍFICOS DE SIMULACIÓN - EXTRAÍDOS de gps.cpp
 */

void GPSSimulation::updateFixedPosition(GPSData& data) {
    data.latitude = simStartLat;
    data.longitude = simStartLon;
}

void GPSSimulation::updateLinearMovement(GPSData& data) {
    float distanceKm = (simSpeed * 1000) / (1000.0f * 3600.0f); // updateInterval was 1000ms
    
    float deltaLat = (distanceKm * 1000.0f / METERS_PER_DEGREE_LAT) * cos(simDirection * DEG_TO_RAD);
    float deltaLon = (distanceKm * 1000.0f / (METERS_PER_DEGREE_LAT * cos(data.latitude * DEG_TO_RAD))) * sin(simDirection * DEG_TO_RAD);
    
    data.latitude += deltaLat;
    data.longitude += deltaLon;
    
    if (!gpsUtils.isValidCoordinate(data.latitude, data.longitude)) {
        simDirection = fmod(simDirection + 180.0f, 360.0f);
    }
}

void GPSSimulation::updateCircularMovement(GPSData& data) {
    float radius = 0.01f;
    float angularSpeed = (simSpeed / EARTH_RADIUS_KM) * RAD_TO_DEG;
    
    float angle = (simStepCounter * angularSpeed * 1000 / 1000.0f); // updateInterval was 1000ms
    
    data.latitude = simStartLat + radius * cos(angle * DEG_TO_RAD);
    data.longitude = simStartLon + radius * sin(angle * DEG_TO_RAD);
    
    simStepCounter++;
}

void GPSSimulation::updateRandomWalk(GPSData& data) {
    float maxStepSize = 0.0001f;
    
    float deltaLat = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    float deltaLon = (random(-1000, 1001) / 1000.0f) * maxStepSize;
    
    float newLat = data.latitude + deltaLat;
    float newLon = data.longitude + deltaLon;
    
    if (gpsUtils.isValidCoordinate(newLat, newLon)) {
        data.latitude = newLat;
        data.longitude = newLon;
    }
}

void GPSSimulation::updateSignalLoss(GPSData& data) {
    unsigned long currentTime = millis();
    
    if (currentTime - signalLossStart >= (signalLossDuration * 1000)) {
        signalLossActive = false;
        data.hasValidFix = true;
        data.satellites = 8;
        Serial.println("[GPS] Señal GPS recuperada");
    } else {
        data.hasValidFix = false;
        data.satellites = random(0, 3);
    }
}

/*
 * UTILIDADES DE SIMULACIÓN - EXTRAÍDAS de gps.cpp
 */

void GPSSimulation::addGPSNoise(GPSData& data) {
    if (!data.hasValidFix) return;
    
    float noiseLat = (random(-300, 301) / 100000.0f) / METERS_PER_DEGREE_LAT;
    float noiseLon = (random(-300, 301) / 100000.0f) / (METERS_PER_DEGREE_LAT * cos(data.latitude * DEG_TO_RAD));
    
    data.latitude += noiseLat;
    data.longitude += noiseLon;
}

void GPSSimulation::updateSatelliteCount(GPSData& data, GPSStatus status) {
    if (status == GPS_STATUS_FIX_3D) {
        int change = random(-1, 2);
        data.satellites = constrain(data.satellites + change, 4, 12);
    } else if (status == GPS_STATUS_SEARCHING) {
        data.satellites = random(0, 4);
    }
}

/*
 * CONFIGURACIÓN DE SIMULACIÓN - EXTRAÍDA de gps.cpp
 */

void GPSSimulation::setSimulationMode(GPSSimulationMode mode) {
    simStepCounter = 0;
    Serial.println("[GPS Simulation] Modo de simulación cambiado a: " + String(mode));
}

void GPSSimulation::setStartPosition(float lat, float lon) {
    if (gpsUtils.isValidCoordinate(lat, lon)) {
        simStartLat = lat;
        simStartLon = lon;
        Serial.println("[GPS Simulation] Posición inicial establecida: " + String(lat, 6) + ", " + String(lon, 6));
    } else {
        Serial.println("[GPS Simulation] ERROR: Coordenadas inválidas");
    }
}

void GPSSimulation::setSimulationSpeed(float speedKmh) {
    simSpeed = constrain(speedKmh, 0.1f, 200.0f);
    Serial.println("[GPS Simulation] Velocidad de simulación: " + String(simSpeed) + " km/h");
}

void GPSSimulation::setSimulationDirection(float degrees) {
    simDirection = fmod(degrees, 360.0f);
    if (simDirection < 0) simDirection += 360.0f;
    Serial.println("[GPS Simulation] Dirección de simulación: " + String(simDirection) + "°");
}

void GPSSimulation::simulateSignalLoss(uint16_t durationSeconds) {
    signalLossActive = true;
    signalLossStart = millis();
    signalLossDuration = durationSeconds;
    Serial.println("[GPS Simulation] Simulando pérdida de señal por " + String(durationSeconds) + " segundos");
}