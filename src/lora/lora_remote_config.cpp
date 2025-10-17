/*
 * LORA_REMOTE_CONFIG.CPP - Configuración Remota y Discovery
 * 
 * Este archivo implementa toda la funcionalidad de configuración remota:
 * - Discovery de dispositivos en la red
 * - Envío de comandos de configuración 
 * - Procesamiento de respuestas
 */

#include "../lora.h"
#include "../gps/gps_manager.h"
#include "../battery/battery_manager.h"

/*
 * DISCOVERY DE DISPOSITIVOS
 */

bool LoRaManager::sendDiscoveryRequest() {
    if (status != LORA_STATUS_READY) {
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Sistema no está listo para discovery");
        }
        return false;
    }
    
    // No necesita payload - solo el message type
    uint8_t emptyPayload = 0;
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Enviando discovery request...");
    }
    
    return sendPacket(MSG_DISCOVERY_REQUEST, &emptyPayload, 1, LORA_BROADCAST_ADDR);
}

bool LoRaManager::sendDiscoveryResponse(uint16_t requestorID) {
    if (status != LORA_STATUS_READY) return false;
    
    // Crear payload con información del dispositivo
    DiscoveryInfo info;
    DeviceConfig config = configManager.getConfig();
    
    // Obtener datos GPS de forma segura
    float lat, lon;
    uint32_t timestamp;
    uint16_t batteryVoltage = 4000; // Default
    
    if (gpsManager.hasValidFix()) {
        lat = gpsManager.getLatitude();
        lon = gpsManager.getLongitude();
        timestamp = gpsManager.getTimestamp();
        batteryVoltage = batteryManager.getVoltage();
    }
    
    info.role = config.role;
    info.gpsInterval = config.gpsInterval;
    info.dataMode = config.dataMode;
    info.region = config.region;
    info.batteryVoltage = batteryVoltage;
    info.uptime = millis() / 1000;  // Segundos desde boot
    
    // Padding
    memset(info.reserved, 0, sizeof(info.reserved));
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Respondiendo a discovery de device " + String(requestorID));
    }
    
    return sendPacket(MSG_DISCOVERY_RESPONSE, (uint8_t*)&info, sizeof(DiscoveryInfo), requestorID);
}

/*
 * COMANDOS REMOTOS
 */

bool LoRaManager::sendRemoteConfigCommand(uint16_t targetID, RemoteCommandType cmdType, uint32_t value, uint32_t sequenceID) {
    if (status != LORA_STATUS_READY) {
        Serial.println("[LoRa] ERROR: Sistema no está listo");
        return false;
    }
    
    // Crear comando
    RemoteConfigCmd cmd;
    cmd.commandType = cmdType;
    cmd.value = value;
    cmd.sequenceID = sequenceID;
    memset(cmd.reserved, 0, sizeof(cmd.reserved));
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Enviando comando " + String(cmdType) + " a device " + String(targetID));
    }
    
    return sendPacket(MSG_CONFIG_CMD, (uint8_t*)&cmd, sizeof(RemoteConfigCmd), targetID);
}

bool LoRaManager::sendRemoteConfigResponse(uint16_t targetID, RemoteCommandType cmdType, bool success, uint32_t sequenceID, uint32_t currentValue, const char* message) {
    if (status != LORA_STATUS_READY) return false;
    
    // Crear respuesta
    RemoteConfigResponse response;
    response.commandType = cmdType;
    response.success = success ? 1 : 0;
    response.sequenceID = sequenceID;
    response.currentValue = currentValue;
    
    // Copiar mensaje con límite de seguridad
    strncpy(response.message, message, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Enviando respuesta a device " + String(targetID));
    }
    
    return sendPacket(MSG_CONFIG_RESPONSE, (uint8_t*)&response, sizeof(RemoteConfigResponse), targetID);
}

/*
 * PROCESAMIENTO DE COMANDOS REMOTOS
 */

bool LoRaManager::processRemoteConfigCommand(const LoRaPacket* packet) {
    if (!packet || packet->messageType != MSG_CONFIG_CMD) return false;
    
    RemoteConfigCmd* cmd = (RemoteConfigCmd*)packet->payload;
    bool success = false;
    uint32_t currentValue = 0;
    String message = "";
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Procesando comando remoto tipo " + String(cmd->commandType) + " de device " + String(packet->sourceID));
    }
    
    // Procesar según tipo de comando
    switch ((RemoteCommandType)cmd->commandType) {
        case REMOTE_CMD_GPS_INTERVAL: {
            if (cmd->value >= 5 && cmd->value <= 3600) {
                // Usar método seguro del ConfigManager
                configManager.setGpsInterval(cmd->value);
                
                success = true;
                currentValue = cmd->value;
                message = "GPS interval cambiado a " + String(cmd->value) + "s";
                
                Serial.println("[CONFIG] GPS interval actualizado remotamente: " + String(cmd->value) + " segundos");
            } else {
                message = "Error: valor fuera de rango (5-3600)";
            }
            break;
        }
        
        case REMOTE_CMD_DATA_MODE: {
            if (cmd->value == 0 || cmd->value == 1) {
                configManager.setDataMode((DataDisplayMode)cmd->value);
                success = true;
                currentValue = cmd->value;
                message = "Modo cambiado a " + String(cmd->value == 0 ? "SIMPLE" : "ADMIN");
                
                Serial.println("[CONFIG] Modo de datos actualizado remotamente: " + String(cmd->value == 0 ? "SIMPLE" : "ADMIN"));
            } else {
                message = "Error: modo inválido (0=SIMPLE, 1=ADMIN)";
            }
            break;
        }
        
        case REMOTE_CMD_STATUS: {
            DeviceConfig config = configManager.getConfig();
            uint16_t batteryVoltage = 4000; // Default
            
            if (gpsManager.hasValidFix()) {
                batteryVoltage = batteryManager.getVoltage();
            }
            
            success = true;
            currentValue = 0;  // No aplica para status
            message = String(config.role) + ",GPS:" + String(config.gpsInterval) + "s,Bat:" + String(batteryVoltage) + "mV";
            break;
        }
        
        case REMOTE_CMD_REBOOT: {
            success = true;
            currentValue = 0;
            message = "Reiniciando...";
            
            // Enviar respuesta primero, luego reboot
            sendRemoteConfigResponse(packet->sourceID, (RemoteCommandType)cmd->commandType, success, cmd->sequenceID, currentValue, message.c_str());
            
            delay(100);  // Dar tiempo para transmitir
            Serial.println("[CONFIG] Reiniciando por comando remoto...");
#if defined(ARDUINO_ARCH_ESP32)
            ESP.restart();
#elif defined(NRF52_SERIES)
            NVIC_SystemReset();
#else
            Serial.println("[WARN] Reinicio no soportado en esta plataforma.");
#endif
            return true;  // No llegar aquí
        }
        
        default:
            message = "Comando no reconocido";
            break;
    }
    
    // Enviar respuesta
    sendRemoteConfigResponse(packet->sourceID, (RemoteCommandType)cmd->commandType, success, cmd->sequenceID, currentValue, message.c_str());
    
    return success;
}

bool LoRaManager::processRemoteConfigResponse(const LoRaPacket* packet) {
    if (!packet || packet->messageType != MSG_CONFIG_RESPONSE) return false;
    
    RemoteConfigResponse* response = (RemoteConfigResponse*)packet->payload;
    
    // Mostrar respuesta al usuario
    String statusStr = response->success ? "OK" : "ERROR";
    Serial.println("[RESPONSE] Device " + String(packet->sourceID) + ": " + String(response->message));
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Respuesta recibida - Comando: " + String(response->commandType) + 
                       ", Status: " + statusStr + 
                       ", Sequence: " + String(response->sequenceID) +
                       ", Value: " + String(response->currentValue));
    }
    
    return true;
}

/*
 * PROCESAMIENTO DE DISCOVERY
 */

bool LoRaManager::processDiscoveryRequest(const LoRaPacket* packet) {
    if (!packet || packet->messageType != MSG_DISCOVERY_REQUEST) return false;
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Discovery request recibido de device " + String(packet->sourceID));
    }
    
    // Responder con nuestra información
    return sendDiscoveryResponse(packet->sourceID);
}

bool LoRaManager::processDiscoveryResponse(const LoRaPacket* packet) {
    if (!packet || packet->messageType != MSG_DISCOVERY_RESPONSE) return false;
    
    DiscoveryInfo* info = (DiscoveryInfo*)packet->payload;
    
    // Convertir role a string
    String roleStr = "UNKNOWN";
    switch (info->role) {
        case ROLE_TRACKER: roleStr = "TRACKER"; break;
        case ROLE_REPEATER: roleStr = "REPEATER"; break;
        case ROLE_RECEIVER: roleStr = "RECEIVER"; break;
        case ROLE_END_NODE_REPEATER: roleStr = "END_NODE_REPEATER"; break;
    }
    
    // Convertir modo a string
    String modeStr = (info->dataMode == 0) ? "SIMPLE" : "ADMIN";
    
    // Convertir región a string
    String regionStr = "US";
    switch (info->region) {
        case REGION_EU: regionStr = "EU"; break;
        case REGION_CH: regionStr = "CH"; break;
        case REGION_AS: regionStr = "AS"; break;
        case REGION_JP: regionStr = "JP"; break;
    }
    
    // Mostrar información del dispositivo encontrado
    Serial.println("[FOUND] Device " + String(packet->sourceID) + ": " + roleStr + 
                   " (RSSI: " + String(stats.lastRSSI) + "dBm" +
                   ", GPS: " + String(info->gpsInterval) + "s" +
                   ", Mode: " + modeStr +
                   ", Region: " + regionStr +
                   ", Battery: " + String(info->batteryVoltage) + "mV" +
                   ", Uptime: " + String(info->uptime) + "s)");
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Discovery response procesado de device " + String(packet->sourceID));
    }
    
    return true;
}
