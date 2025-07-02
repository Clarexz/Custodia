/*
 * BATTERY_MANAGER.H - Sistema de Monitoreo de Batería
 * 
 * EXTRAÍDO de gps.cpp - Sistema independiente para battery monitoring
 * según packet format: [deviceID, latitude, longitude, batteryvoltage, timestamp]
 */

#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>

/*
 * CONSTANTES DE BATERÍA
 */
#define BATTERY_MAX_VOLTAGE 4200  // mV - Batería completamente cargada (4.2V)
#define BATTERY_MIN_VOLTAGE 3200  // mV - Batería descargada (3.2V)
#define BATTERY_DRAIN_RATE 0.1f   // mV por minuto de simulación

/*
 * CLASE PRINCIPAL - BatteryManager
 */
class BatteryManager {
private:
    // Variables de estado
    uint16_t currentVoltage;
    unsigned long startTime;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    BatteryManager();
    ~BatteryManager();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    
    // Inicialización del sistema
    void begin();
    void begin(uint16_t initialVoltage);
    
    // Actualización principal
    void update();
    
    /*
     * OBTENCIÓN DE DATOS
     */
    
    // Obtener voltage actual en mV
    uint16_t getVoltage();
    
    // Obtener porcentaje de batería (0-100%)
    uint8_t getPercentage();
    
    // Verificar estado de batería
    bool isLow();
    bool isCritical();
    
    /*
     * CONFIGURACIÓN
     */
    
    // Configurar voltage inicial
    void setVoltage(uint16_t voltage);
    
    // Reset del sistema (batería llena)
    void reset();
    
    /*
     * DIAGNÓSTICO
     */
    
    // Información de debug
    void printStatus();
    
    // Tiempo de funcionamiento
    unsigned long getUptimeMinutes();
};

/*
 * INSTANCIA GLOBAL
 */
extern BatteryManager batteryManager;

#endif