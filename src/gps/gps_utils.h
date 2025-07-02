/*
 * GPS_UTILS.H - Utilidades y Cálculos Geográficos GPS
 * 
 * EXTRAÍDO de gps.h/cpp - Funciones de validación y cálculos geográficos
 */

#ifndef GPS_UTILS_H
#define GPS_UTILS_H

#include <Arduino.h>

/*
 * CLASE DE UTILIDADES GPS
 */
class GPSUtils {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    GPSUtils();
    ~GPSUtils();
    
    /*
     * VALIDACIÓN DE COORDENADAS
     */
    
    // Validar coordenadas individuales
    bool isValidLatitude(float lat);
    bool isValidLongitude(float lon);
    bool isValidCoordinate(float lat, float lon);
    
    /*
     * CÁLCULOS GEOGRÁFICOS
     */
    
    // Calcular distancia entre dos puntos (Haversine)
    float calculateDistance(float lat1, float lon1, float lat2, float lon2);
    
    // Calcular bearing (rumbo) entre dos puntos
    float calculateBearing(float lat1, float lon1, float lat2, float lon2);
    
    /*
     * CONVERSIONES
     */
    
    // Convertir grados a radianes
    float degToRad(float degrees);
    
    // Convertir radianes a grados
    float radToDeg(float radians);
};

/*
 * INSTANCIA GLOBAL
 */
extern GPSUtils gpsUtils;

#endif