/*
 * BATTERY_MANAGER.CPP - Implementación del Sistema de Monitoreo de Batería
 * 
 * EXTRAÍDO de gps.cpp - Funciones updateBatteryVoltage, getBatteryVoltage, 
 * getBatteryPercentage y toda la lógica de battery monitoring
 */

#include "battery_manager.h"

/*
 * INSTANCIA GLOBAL
 */
BatteryManager batteryManager;

/*
 * CONSTRUCTOR
 */
BatteryManager::BatteryManager() {
    currentVoltage = BATTERY_MAX_VOLTAGE;
    startTime = 0;
}

/*
 * DESTRUCTOR
 */
BatteryManager::~BatteryManager() {
    // Limpieza si es necesaria
}

/*
 * INICIALIZACIÓN DEL SISTEMA
 */
void BatteryManager::begin() {
    begin(BATTERY_MAX_VOLTAGE - random(0, 200)); // 4.0-4.2V inicial
}

void BatteryManager::begin(uint16_t initialVoltage) {
    currentVoltage = constrain(initialVoltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
    startTime = millis();
    
    Serial.println("[Battery] Sistema inicializado");
    Serial.println("[Battery] Voltage inicial: " + String(currentVoltage) + " mV");
}

/*
 * ACTUALIZACIÓN PRINCIPAL - EXTRAÍDO de updateBatteryVoltage()
 */
void BatteryManager::update() {
    // Simular descarga gradual de batería
    unsigned long runningTime = millis() - startTime;
    float minutesRunning = runningTime / 60000.0f;
    
    // Calcular voltage basado en tiempo transcurrido
    float drainedVoltage = minutesRunning * BATTERY_DRAIN_RATE;
    currentVoltage = BATTERY_MAX_VOLTAGE - (uint16_t)drainedVoltage;
    
    // Asegurar que no baje del mínimo
    if (currentVoltage < BATTERY_MIN_VOLTAGE) {
        currentVoltage = BATTERY_MIN_VOLTAGE;
    }
    
    // Agregar pequeña variación aleatoria (±10mV)
    int variation = random(-10, 11);
    currentVoltage = constrain(currentVoltage + variation, 
                              BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
}

/*
 * OBTENCIÓN DE DATOS - EXTRAÍDO de getBatteryVoltage()
 */
uint16_t BatteryManager::getVoltage() {
    return currentVoltage;
}

/*
 * EXTRAÍDO de getBatteryPercentage()
 */
uint8_t BatteryManager::getPercentage() {
    if (currentVoltage <= BATTERY_MIN_VOLTAGE) return 0;
    if (currentVoltage >= BATTERY_MAX_VOLTAGE) return 100;
    
    return ((currentVoltage - BATTERY_MIN_VOLTAGE) * 100) / 
           (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);
}

/*
 * VERIFICACIÓN DE ESTADO
 */
bool BatteryManager::isLow() {
    return getPercentage() < 20;
}

bool BatteryManager::isCritical() {
    return getPercentage() < 10;
}

/*
 * CONFIGURACIÓN
 */
void BatteryManager::setVoltage(uint16_t voltage) {
    currentVoltage = constrain(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
}

void BatteryManager::reset() {
    currentVoltage = BATTERY_MAX_VOLTAGE - random(0, 200);
    startTime = millis();
    Serial.println("[Battery] Reset - Voltage: " + String(currentVoltage) + " mV");
}

/*
 * DIAGNÓSTICO
 */
void BatteryManager::printStatus() {
    Serial.println("[Battery] Voltage: " + String(currentVoltage) + " mV (" + 
                   String(getPercentage()) + "%)");
    Serial.println("[Battery] Tiempo funcionamiento: " + String(getUptimeMinutes()) + " minutos");
}

unsigned long BatteryManager::getUptimeMinutes() {
    return (millis() - startTime) / 60000;
}