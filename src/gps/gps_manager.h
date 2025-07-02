/*
 * GPS_MANAGER.H - Sistema GPS Coordinador (sin battery)
 * 
 * MODULARIZADO de gps.h - Solo coordinación principal, sin simulación ni utils
 * Battery monitoring ahora está en battery_manager.h
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>

/*
 * FORWARD DECLARATIONS
 */
class GPSSimulation;
class GPSUtils;

/*
 * ESTRUCTURAS DE DATOS GPS - SIN BATTERY
 */
struct GPSData {
    float latitude;          // Latitud en grados decimales (-90 a +90)
    float longitude;         // Longitud en grados decimales (-180 a +180)
    bool hasValidFix;        // True si tenemos posición válida
    uint32_t timestamp;      // Timestamp Unix de la lectura
    uint8_t satellites;      // Número de satélites (solo para diagnóstico interno)
};

/*
 * ESTADOS DEL GPS
 */
enum GPSStatus {
    GPS_STATUS_OFF = 0,      // GPS apagado/no inicializado
    GPS_STATUS_SEARCHING = 1, // Buscando satélites
    GPS_STATUS_FIX_2D = 2,   // Fix 2D obtenido
    GPS_STATUS_FIX_3D = 3,   // Fix 3D obtenido (con altitud)
    GPS_STATUS_ERROR = 4     // Error en el módulo GPS
};

/*
 * MODOS DE SIMULACIÓN GPS
 */
enum GPSSimulationMode {
    GPS_SIM_FIXED = 0,       // Posición fija (para pruebas estáticas)
    GPS_SIM_LINEAR = 1,      // Movimiento lineal (simula vehículo)
    GPS_SIM_CIRCULAR = 2,    // Movimiento circular (simula patrullaje)
    GPS_SIM_RANDOM_WALK = 3, // Caminata aleatoria (simula persona)
    GPS_SIM_SIGNAL_LOSS = 4  // Simula pérdida/recuperación de señal
};

/*
 * CLASE PRINCIPAL - GPSManager (Coordinador)
 */
class GPSManager {
private:
    // Configuración del GPS
    GPSData currentData;
    GPSStatus status;
    GPSSimulationMode simMode;
    
    // Variables de control
    unsigned long lastUpdateTime;
    uint16_t updateInterval;
    
    // Componentes modulares
    GPSSimulation* simulation;
    GPSUtils* utils;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    GPSManager();
    ~GPSManager();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    
    // Inicialización del sistema GPS
    void begin();
    void begin(GPSSimulationMode mode);
    
    // Control del GPS
    void update();
    void enable();
    void disable();
    void reset();
    
    /*
     * CONFIGURACIÓN
     */
    
    // Configurar modo de simulación
    void setSimulationMode(GPSSimulationMode mode);
    void setStartPosition(float lat, float lon);
    void setSimulationSpeed(float speedKmh);
    void setUpdateInterval(uint16_t intervalMs);
    void setSimulationDirection(float degrees);
    void simulateSignalLoss(uint16_t durationSeconds);
    
    /*
     * OBTENCIÓN DE DATOS
     */
    
    // Obtener datos GPS actuales
    GPSData getCurrentData();
    GPSData* getCurrentDataPtr();
    
    // Obtener datos específicos
    float getLatitude();
    float getLongitude();
    bool hasValidFix();
    uint32_t getTimestamp();
    uint8_t getSatelliteCount();
    
    // Estado del GPS
    GPSStatus getStatus();
    String getStatusString();
    bool isEnabled();
    
    /*
     * FORMATEO Y CONVERSIÓN DE DATOS
     */
    
    // Formatear coordenadas para transmisión
    String formatCoordinates();              // Formato: "lat,lon"
    String formatForTransmission();          // Formato: "lat,lon,timestamp"
    String formatPacketWithDeviceID(uint16_t deviceID);  // Formato: "deviceID,lat,lon,battery,timestamp"
    
    // Conversiones básicas
    String latitudeToString(int precision = 6);
    String longitudeToString(int precision = 6);
    
    /*
     * VALIDACIÓN Y UTILIDADES
     */
    
    // Validar coordenadas
    bool isValidLatitude(float lat);
    bool isValidLongitude(float lon);
    bool isValidCoordinate(float lat, float lon);
    
    // Calcular distancia entre puntos
    float distanceTo(float lat, float lon);
    float bearingTo(float lat, float lon);
    
    /*
     * DEBUGGING Y DIAGNÓSTICO
     */
    
    // Información de debug
    void printCurrentData();
    void printStatus();
    void printSimulationInfo();
    
    // Estadísticas
    uint32_t getTotalUpdates();
    unsigned long getUptimeSeconds();
};

/*
 * INSTANCIA GLOBAL
 */
extern GPSManager gpsManager;

/*
 * CONSTANTES ÚTILES
 */

// Coordenadas de ejemplo (Reynosa, Tamaulipas, México)
#define DEFAULT_LATITUDE  25.302677f
#define DEFAULT_LONGITUDE -98.277664f

// Límites de validación
#define MIN_LATITUDE  -90.0f
#define MAX_LATITUDE   90.0f
#define MIN_LONGITUDE -180.0f
#define MAX_LONGITUDE  180.0f

// Configuración de simulación por defecto
#define DEFAULT_SIM_SPEED         5.0f    // km/h
#define DEFAULT_UPDATE_INTERVAL   1000    // ms

#endif