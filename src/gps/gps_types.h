/*
 * GPS_TYPES.H - Datos y constantes compartidas del subsistema GPS
 */

#ifndef GPS_TYPES_H
#define GPS_TYPES_H

#include <stdint.h>

/*
 * ESTRUCTURAS DE DATOS GPS
 */
struct GPSData {
    float latitude;          // Latitud en grados decimales (-90 a +90)
    float longitude;         // Longitud en grados decimales (-180 a +180)
    bool hasValidFix;        // True si tenemos posición válida
    uint32_t timestamp;      // Timestamp Unix de la lectura
    uint8_t satellites;      // Número de satélites (diagnóstico)
};

/*
 * ESTADOS DEL GPS
 */
enum GPSStatus {
    GPS_STATUS_OFF = 0,
    GPS_STATUS_SEARCHING = 1,
    GPS_STATUS_FIX_2D = 2,
    GPS_STATUS_FIX_3D = 3,
    GPS_STATUS_ERROR = 4
};

/*
 * CONSTANTES DE CONFIGURACIÓN
 */
#define DEFAULT_LATITUDE  25.302677f
#define DEFAULT_LONGITUDE -98.277664f

#define MIN_LATITUDE  -90.0f
#define MAX_LATITUDE   90.0f
#define MIN_LONGITUDE -180.0f
#define MAX_LONGITUDE  180.0f

#define DEFAULT_SIM_SPEED         5.0f    // km/h
#define DEFAULT_UPDATE_INTERVAL   1000    // ms

#endif // GPS_TYPES_H
