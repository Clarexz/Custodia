/*
 * ADMIN_DISPLAY.CPP - Modo de visualización ADMIN
 */

#include "admin_display.h"
#include "../config/config_manager.h"
#include "../gps/gps_manager.h"
#include "../lora.h"
#include "../battery/battery_manager.h"

// Instancia global
AdminDisplay adminDisplay;

/*
 * CONSTRUCTOR
 */
AdminDisplay::AdminDisplay() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
AdminDisplay::~AdminDisplay() {
    // Cleanup si es necesario
}

/*
 * REEMPLAZAR las 3 funciones show*Output() en admin_display.cpp
 * 
 * Versión SIN referencias externas que no compilan
 */

/*
 * TRACKER - Simplificado
 */
void AdminDisplay::showTrackerOutput(uint16_t deviceID, bool sent) {
    // Obtener datos necesarios
    GPSData gpsData = gpsManager.getCurrentData();
    LoRaStats stats = loraManager.getStats();
    
    String activeChannel = configManager.getActiveChannelName();
    
    // STATUS LINE compacta
    Serial.println("[TRACKER] GPS Sent | ID:" + String(deviceID) + 
                  " | Coord:" + String(gpsData.latitude, 6) + "," + String(gpsData.longitude, 6) +
                  " | Bat:" + String(batteryManager.getVoltage()) + "mV" +
                  " | TX:" + String(stats.packetsSent) +
                  " | Channel:" + activeChannel +
                  " | Status:" + String(sent ? "OK" : "FAIL"));
}

/*
 * REPEATER - Simplificado  
 */
void AdminDisplay::showRepeaterOutput() {
    // Obtener estadísticas
    LoRaStats stats = loraManager.getStats();
    
    // Variables estáticas para control de output
    static uint32_t lastUpdateTime = 0;
    static uint32_t lastRetxCount = 0;
    uint32_t currentTime = millis();
    
    // Mostrar solo cuando hay nueva actividad O cada 10 segundos
    bool hasNewActivity = (stats.rebroadcasts > lastRetxCount);
    bool timeForUpdate = (currentTime - lastUpdateTime > 10000);
    
    if (hasNewActivity || timeForUpdate) {
        String activeChannel = configManager.getActiveChannelName();
        
        // STATUS LINE compacta
        Serial.println("[REPEATER] Mesh Router | RX:" + String(stats.packetsReceived) + 
                      " | TX:" + String(stats.packetsSent) +
                      " | Retx:" + String(stats.rebroadcasts) +
                      " | Dupl:" + String(stats.duplicatesIgnored) +
                      " | Drops:" + String(stats.hopLimitReached) +
                      " | Channel:" + activeChannel);
        
        lastUpdateTime = currentTime;
        lastRetxCount = stats.rebroadcasts;
    }
}

/*
 * RECEIVER - Simplificado
 */
void AdminDisplay::showReceiverOutput() {
    // Obtener estadísticas actuales
    LoRaStats stats = loraManager.getStats();
    
    // Variables estáticas para detectar cambios
    static uint32_t lastPacketCount = 0;
    static uint32_t lastUpdateTime = 0;
    uint32_t currentTime = millis();
    
    // Mostrar status compacto cada 5 segundos O cuando hay nuevos packets
    bool hasNewPackets = (stats.packetsReceived > lastPacketCount);
    bool timeForUpdate = (currentTime - lastUpdateTime > 5000);
    
    if (hasNewPackets || timeForUpdate) {
        String activeChannel = configManager.getActiveChannelName();
        
        // STATUS LINE - 1 línea compacta con toda la info esencial
        Serial.println("[RECEIVER] Mesh Monitor | RX:" + String(stats.packetsReceived) + 
                      " | Retx:" + String(stats.rebroadcasts) + 
                      " | Dupl:" + String(stats.duplicatesIgnored) + 
                      " | Signal:" + String(stats.lastRSSI) + "dBm/" + String(stats.lastSNR) + "dB" +
                      " | Channel:" + activeChannel);
        
        lastUpdateTime = currentTime;
    }
    
    // Mostrar GPS data SOLO cuando hay nuevos packets
    if (hasNewPackets) {
        Serial.println("[GPS] New data received | RSSI:" + String(stats.lastRSSI) + "dBm SNR:" + String(stats.lastSNR) + "dB");
        lastPacketCount = stats.packetsReceived;
    }
}