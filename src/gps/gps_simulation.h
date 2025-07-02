/*
 * GPS_SIMULATION.H - Sistema de Simulación GPS
 * 
 * EXTRAÍDO de gps.h/cpp - Todos los modos de simulación
 */

#ifndef GPS_SIMULATION_H
#define GPS_SIMULATION_H

#include <Arduino.h>
#include "gps_manager.h"

/*
 * CLASE DE SIMULACIÓN GPS
 */
class GPSSimulation {
private:
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
    
    // Modos específicos de simulación
    void updateFixedPosition(GPSData& data);
    void updateLinearMovement(GPSData& data);
    void updateCircularMovement(GPSData& data);
    void updateRandomWalk(GPSData& data);
    void updateSignalLoss(GPSData& data);
    
    // Utilidades de simulación
    void addGPSNoise(GPSData& data);
    void updateSatelliteCount(GPSData& data, GPSStatus status);
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    GPSSimulation();
    ~GPSSimulation();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    
    // Inicialización
    void begin(GPSSimulationMode mode);
    
    // Actualización principal
    GPSData updateSimulation(GPSData currentData, GPSSimulationMode mode);
    
    /*
     * CONFIGURACIÓN DE SIMULACIÓN
     */
    
    // Cambiar modo de simulación
    void setSimulationMode(GPSSimulationMode mode);
    
    // Configurar posición inicial para simulaciones
    void setStartPosition(float lat, float lon);
    
    // Configurar parámetros de simulación
    void setSimulationSpeed(float speedKmh);
    void setSimulationDirection(float degrees);
    
    // Configurar simulación de pérdida de señal
    void simulateSignalLoss(uint16_t durationSeconds);
};

/*
 * INSTANCIA GLOBAL
 */
extern GPSSimulation gpsSimulation;

/*
 * CONSTANTES MATEMÁTICAS Y DE CONVERSIÓN
 */
#define EARTH_RADIUS_KM 6371.0f
#define PI 3.14159265359f
#define DEG_TO_RAD (PI/180.0f)
#define RAD_TO_DEG (180.0f/PI)
#define METERS_PER_DEGREE_LAT 111320.0f

#endif