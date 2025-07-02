/*
 * TRACKER_ROLE.CPP - Lógica específica del rol TRACKER
 */

#include "tracker_role.h"
#include "../config/config_manager.h"
#include "../gps/gps_manager.h"
#include "../lora.h"
#include "../display/display_manager.h"
#include "../battery/battery_manager.h"

// Pin del LED para indicadores visuales
#define LED_PIN 21

// Instancia global
TrackerRole trackerRole;

/*
 * CONSTRUCTOR
 */
TrackerRole::TrackerRole() {
    lastGPSTransmission = 0;
    lastStatusCheck = 0;
}

/*
 * DESTRUCTOR
 */
TrackerRole::~TrackerRole() {
    // Cleanup si es necesario
}

/*
 * LÓGICA PRINCIPAL DEL TRACKER
 */
void TrackerRole::handleMode() {
    DeviceConfig config = configManager.getConfig();
    unsigned long currentTime = millis();
    
    // Verificación periódica del estado de LoRa (cada 30 segundos)
    if (currentTime - lastStatusCheck >= 30000) {
        lastStatusCheck = currentTime;
        LoRaStatus loraStatus = loraManager.getStatus();
        
        if (loraStatus == LORA_STATUS_ERROR || loraStatus == LORA_STATUS_INIT) {
            Serial.println("[TRACKER] WARNING: LoRa no está listo. Estado: " + loraManager.getStatusString());
            Serial.println("[TRACKER] Reintentando inicialización...");
            // initializeLoRaForRole(); // Se manejará desde RoleManager
            return;
        }
        
        // Mostrar estadísticas mesh solo en modo ADMIN
        if (configManager.isAdminMode()) {
            loraManager.printMeshStats();
        }
    }
    
    // Verificar si es tiempo de transmitir posición GPS
    if (currentTime - lastGPSTransmission >= (config.gpsInterval * 1000)) {
        lastGPSTransmission = currentTime;
        
        // LED parpadeo rápido para indicar transmisión GPS
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        
        // Obtener datos GPS actuales
        GPSData gpsData = gpsManager.getCurrentData();
        
        if (gpsData.hasValidFix) {
            // Obtener datos para transmisión
            float lat = gpsData.latitude;
            float lon = gpsData.longitude;
            uint16_t battery = batteryManager.getVoltage();
            uint32_t timestamp = gpsData.timestamp;
            
            // Verificar estado de LoRa antes de transmitir
            if (loraManager.getStatus() != LORA_STATUS_READY) {
                Serial.println("[TRACKER] WARNING: LoRa no está listo para transmitir");
                Serial.println("[TRACKER] Estado actual: " + loraManager.getStatusString());
                return;
            }
            
            // Transmitir via LoRa
            bool sent = loraManager.sendGPSData(lat, lon, timestamp);
            
            // Mostrar output según modo configurado
            displayManager.showTrackerOutput(config.deviceID, lat, lon, battery, timestamp, sent);
            
        } else {
            Serial.println("[TRACKER] Sin fix GPS - Esperando señal...");
        }
    }
    
    delay(100);
}