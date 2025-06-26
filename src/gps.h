/*
 * GPS.H - Sistema GPS para Custom Meshtastic GPS Tracker + Battery Monitoring
 * 
 * ACTUALIZACIÓN: Agregando soporte para battery voltage para cumplir con
 * el formato de packet requerido: [deviceID, latitude, longitude, batteryvoltage, timestamp]
 */

#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

/*
 * ESTRUCTURAS DE DATOS GPS - ACTUALIZADA
 */

// Estructura principal para almacenar datos GPS + Battery
struct GPSData {
    float latitude;          // Latitud en grados decimales (-90 a +90)
    float longitude;         // Longitud en grados decimales (-180 a +180)
    bool hasValidFix;        // True si tenemos posición válida
    uint32_t timestamp;      // Timestamp Unix de la lectura
    uint16_t batteryVoltage; // NUEVO: Voltage de batería en mV
    uint8_t satellites;      // Número de satélites (solo para diagnóstico interno)
};

/*
 * MODOS DE SIMULACIÓN GPS (sin cambios)
 */
enum GPSSimulationMode {
    GPS_SIM_FIXED = 0,       // Posición fija (para pruebas estáticas)
    GPS_SIM_LINEAR = 1,      // Movimiento lineal (simula vehículo)
    GPS_SIM_CIRCULAR = 2,    // Movimiento circular (simula patrullaje)
    GPS_SIM_RANDOM_WALK = 3, // Caminata aleatoria (simula persona)
    GPS_SIM_SIGNAL_LOSS = 4  // Simula pérdida/recuperación de señal
};

/*
 * ESTADOS DEL GPS (sin cambios)
 */
enum GPSStatus {
    GPS_STATUS_OFF = 0,      // GPS apagado/no inicializado
    GPS_STATUS_SEARCHING = 1, // Buscando satélites
    GPS_STATUS_FIX_2D = 2,   // Fix 2D obtenido
    GPS_STATUS_FIX_3D = 3,   // Fix 3D obtenido (con altitud)
    GPS_STATUS_ERROR = 4     // Error en el módulo GPS
};

/*
 * CLASE PRINCIPAL - GPSManager
 * 
 * ACTUALIZADA: Ahora incluye gestión de battery voltage
 */
class GPSManager {
private:
    // Configuración del GPS
    GPSData currentData;
    GPSStatus status;
    GPSSimulationMode simMode;
    
    // Variables para simulación
    unsigned long lastUpdateTime;
    uint16_t updateInterval;
    
    // Variables para diferentes modos de simulación
    float simStartLat, simStartLon;
    float simDirection;
    float simSpeed;
    uint16_t simStepCounter;
    
    // Variables para simulación de pérdida de señal
    bool signalLossActive;
    unsigned long signalLossStart;
    uint16_t signalLossDuration;
    
    /*
     * MÉTODOS PRIVADOS PARA SIMULACIÓN
     */
    
    // Actualiza posición según el modo de simulación
    void updateSimulation();
    
    // NUEVO: Actualiza battery voltage
    void updateBatteryVoltage();
    
    // Modos específicos de simulación
    void updateFixedPosition();
    void updateLinearMovement();
    void updateCircularMovement();
    void updateRandomWalk();
    void updateSignalLoss();
    
    // Utilidades de simulación
    void addGPSNoise();
    void updateSatelliteCount();
    float calculateDistance(float lat1, float lon1, float lat2, float lon2);
    float calculateBearing(float lat1, float lon1, float lat2, float lon2);
    
    /*
     * MÉTODOS PRIVADOS PARA GPS REAL (FUTURO)
     */
    void initHardwareGPS();
    bool parseNMEA(String nmeaSentence);
    void checkGPSHardware();
    
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
     * CONFIGURACIÓN DE SIMULACIÓN
     */
    
    // Cambiar modo de simulación
    void setSimulationMode(GPSSimulationMode mode);
    
    // Configurar posición inicial para simulaciones
    void setStartPosition(float lat, float lon);
    
    // Configurar parámetros de simulación
    void setSimulationSpeed(float speedKmh);
    void setUpdateInterval(uint16_t intervalMs);
    void setSimulationDirection(float degrees);
    
    // Configurar simulación de pérdida de señal
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
    
    // NUEVO: Obtener datos de battery
    uint16_t getBatteryVoltage();
    uint8_t getBatteryPercentage();
    
    // Estado del GPS (solo para diagnóstico)
    GPSStatus getStatus();
    String getStatusString();
    bool isEnabled();
    uint8_t getSatelliteCount();
    
    /*
     * FORMATEO Y CONVERSIÓN DE DATOS - ACTUALIZADO
     */
    
    // Formatear coordenadas para transmisión
    String formatCoordinates();              // Formato: "lat,lon"
    String formatForTransmission();          // Formato: "lat,lon,battery,timestamp"
    
    // NUEVO: Formatear packet completo con deviceID
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