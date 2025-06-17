/*
 * GPS.H - Sistema GPS para Custom Meshtastic GPS Tracker
 * 
 * Este archivo define la interfaz del sistema GPS que maneja la obtención
 * de coordenadas geográficas. Incluye tanto simulación como preparación
 * para módulos GPS reales.
 * 
 * Características:
 * - GPS simulado con múltiples modos de operación
 * - Estructura preparada para GPS real (UART/NMEA)
 * - Validación automática de coordenadas
 * - Manejo de estados de fix GPS
 */

#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

/*
 * ESTRUCTURAS DE DATOS GPS
 */

// Estructura principal para almacenar datos GPS (SIMPLIFICADA)
struct GPSData {
    float latitude;          // Latitud en grados decimales (-90 a +90)
    float longitude;         // Longitud en grados decimales (-180 a +180)
    bool hasValidFix;        // True si tenemos posición válida
    uint32_t timestamp;      // Timestamp Unix de la lectura
    uint8_t satellites;      // Número de satélites (solo para diagnóstico interno)
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
 * CLASE PRINCIPAL - GPSManager
 * 
 * Maneja toda la funcionalidad GPS incluyendo simulación,
 * lectura de coordenadas, validación y formateo de datos.
 */
class GPSManager {
private:
    // Configuración del GPS
    GPSData currentData;
    GPSStatus status;
    GPSSimulationMode simMode;
    
    // Variables para simulación
    unsigned long lastUpdateTime;
    uint16_t updateInterval;     // Intervalo entre actualizaciones en ms
    
    // Variables para diferentes modos de simulación
    float simStartLat, simStartLon;  // Posición inicial de simulación
    float simDirection;              // Dirección actual (en grados)
    float simSpeed;                  // Velocidad de simulación (km/h)
    uint16_t simStepCounter;         // Contador para patrones
    
    // Variables para simulación de pérdida de señal
    bool signalLossActive;
    unsigned long signalLossStart;
    uint16_t signalLossDuration;
    
    /*
     * MÉTODOS PRIVADOS PARA SIMULACIÓN
     */
    
    // Actualiza posición según el modo de simulación
    void updateSimulation();
    
    // Modos específicos de simulación
    void updateFixedPosition();
    void updateLinearMovement();
    void updateCircularMovement();
    void updateRandomWalk();
    void updateSignalLoss();
    
    // Utilidades de simulación
    void addGPSNoise();              // Agrega ruido realista a coordenadas
    void updateSatelliteCount();     // Simula cambios en número de satélites
    float calculateDistance(float lat1, float lon1, float lat2, float lon2);
    float calculateBearing(float lat1, float lon1, float lat2, float lon2);
    
    /*
     * MÉTODOS PRIVADOS PARA GPS REAL (FUTURO)
     */
    
    // Para implementación futura con módulo GPS físico
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
    void update();                   // Debe llamarse en el loop principal
    void enable();                   // Enciende el GPS
    void disable();                  // Apaga el GPS para ahorrar energía
    void reset();                    // Reinicia el GPS
    
    /*
     * CONFIGURACIÓN DE SIMULACIÓN
     */
    
    // Cambiar modo de simulación
    void setSimulationMode(GPSSimulationMode mode);
    
    // Configurar posición inicial para simulaciones
    void setStartPosition(float lat, float lon);
    
    // Configurar parámetros de simulación
    void setSimulationSpeed(float speedKmh);      // Velocidad para movimiento
    void setUpdateInterval(uint16_t intervalMs);  // Frecuencia de actualización
    void setSimulationDirection(float degrees);   // Dirección inicial
    
    // Configurar simulación de pérdida de señal
    void simulateSignalLoss(uint16_t durationSeconds);
    
    /*
     * OBTENCIÓN DE DATOS (SIMPLIFICADO)
     */
    
    // Obtener datos GPS actuales
    GPSData getCurrentData();
    GPSData* getCurrentDataPtr();    // Para acceso directo sin copia
    
    // Obtener datos específicos (solo esenciales)
    float getLatitude();
    float getLongitude();
    bool hasValidFix();
    uint32_t getTimestamp();
    
    // Estado del GPS (solo para diagnóstico)
    GPSStatus getStatus();
    String getStatusString();
    bool isEnabled();
    uint8_t getSatelliteCount();     // Solo para debug interno
    
    /*
     * FORMATEO Y CONVERSIÓN DE DATOS (SIMPLIFICADO)
     */
    
    // Formatear coordenadas para transmisión
    String formatCoordinates();              // Formato: "lat,lon"
    String formatForTransmission();          // Formato: "deviceID,lat,lon,timestamp"
    
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
    float distanceTo(float lat, float lon);  // Distancia en metros
    float bearingTo(float lat, float lon);   // Rumbo en grados
    
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
 * 
 * Se declara aquí y se define en gps.cpp para uso en todo el proyecto
 */
extern GPSManager gpsManager;

/*
 * CONSTANTES ÚTILES (SIMPLIFICADAS)
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