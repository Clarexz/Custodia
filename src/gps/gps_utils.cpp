/*
 * GPS_UTILS.CPP - Implementación de Utilidades y Cálculos Geográficos
 * 
 * EXTRAÍDO de gps.cpp - Funciones calculateDistance, calculateBearing, 
 * validaciones y cálculos matemáticos
 */

#include "gps_utils.h"
#include "gps_manager.h"
#include <math.h>

/*
 * CONSTANTES MATEMÁTICAS Y DE CONVERSIÓN
 */
#define EARTH_RADIUS_KM 6371.0f
#define PI 3.14159265359f
#define DEG_TO_RAD (PI/180.0f)
#define RAD_TO_DEG (180.0f/PI)

/*
 * INSTANCIA GLOBAL
 */
GPSUtils gpsUtils;

/*
 * CONSTRUCTOR
 */
GPSUtils::GPSUtils() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
GPSUtils::~GPSUtils() {
    // Limpieza si es necesaria
}

/*
 * VALIDACIÓN DE COORDENADAS - EXTRAÍDO de gps.cpp
 */

bool GPSUtils::isValidLatitude(float lat) {
    return (lat >= MIN_LATITUDE && lat <= MAX_LATITUDE);
}

bool GPSUtils::isValidLongitude(float lon) {
    return (lon >= MIN_LONGITUDE && lon <= MAX_LONGITUDE);
}

bool GPSUtils::isValidCoordinate(float lat, float lon) {
    return isValidLatitude(lat) && isValidLongitude(lon);
}

/*
 * CÁLCULOS GEOGRÁFICOS - EXTRAÍDOS de gps.cpp
 */

// EXTRAÍDO de calculateDistance()
float GPSUtils::calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * DEG_TO_RAD;
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    
    float a = sin(dLat/2) * sin(dLat/2) + 
              cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) * 
              sin(dLon/2) * sin(dLon/2);
    
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return EARTH_RADIUS_KM * c * 1000.0f;
}

// EXTRAÍDO de calculateBearing()
float GPSUtils::calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    float lat1Rad = lat1 * DEG_TO_RAD;
    float lat2Rad = lat2 * DEG_TO_RAD;
    
    float y = sin(dLon) * cos(lat2Rad);
    float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
    
    float bearing = atan2(y, x) * RAD_TO_DEG;
    return fmod(bearing + 360.0f, 360.0f);
}

/*
 * CONVERSIONES
 */

float GPSUtils::degToRad(float degrees) {
    return degrees * DEG_TO_RAD;
}

float GPSUtils::radToDeg(float radians) {
    return radians * RAD_TO_DEG;
}