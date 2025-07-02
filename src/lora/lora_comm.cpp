/*
 * LORA_COMM.CPP - Transmisión y Recepción Básica
 * 
 * ACTUALIZADO: Agregado manejo de mensajes de configuración remota y discovery
 */

#include "../lora.h"

/*
 * ENVÍO DE DATOS GPS (sin cambios)
 */
bool LoRaManager::sendGPSData(float latitude, float longitude, uint32_t timestamp) {
    return sendGPSData(latitude, longitude, timestamp, LORA_BROADCAST_ADDR);
}

bool LoRaManager::sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID) {
    // Crear payload GPS
    GPSPayload gpsPayload;
    gpsDataToPayload(latitude, longitude, timestamp, &gpsPayload);
    
    // Enviar como packet GPS
    return sendPacket(MSG_GPS_DATA, (uint8_t*)&gpsPayload, sizeof(GPSPayload), destinationID);
}

/*
 * ENVÍO DE PACKET GENÉRICO (sin cambios)
 */
bool LoRaManager::sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength) {
    return sendPacket(msgType, payload, payloadLength, LORA_BROADCAST_ADDR);
}

bool LoRaManager::sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID) {
    if (status != LORA_STATUS_READY) {
        // SOLO mostrar error en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Sistema no está listo para transmitir");
        }
        return false;
    }
    
    if (payloadLength > LORA_MAX_PAYLOAD_SIZE) {
        // SOLO mostrar error en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Payload demasiado grande");
        }
        return false;
    }
    
    // Crear packet
    LoRaPacket packet;
    packet.messageType = msgType;
    packet.sourceID = deviceID;
    packet.destinationID = destinationID;
    packet.hops = 0;  // Packet original
    packet.maxHops = MESHTASTIC_MAX_HOPS;  // Máximo saltos
    packet.packetID = ++packetCounter;
    packet.payloadLength = payloadLength;
    
    // Copiar payload
    memcpy(packet.payload, payload, payloadLength);
    
    // Calcular checksum
    packet.checksum = calculateChecksum(&packet);
    
    // Agregar a seen packets para evitar retransmitirlos
    addToRecentPackets(packet.sourceID, packet.packetID);
    
    // Cambiar a modo transmisión
    status = LORA_STATUS_TRANSMITTING;
    
    // Transmitir packet
    unsigned long startTime = millis();
    int state = radio.transmit((uint8_t*)&packet, sizeof(LoRaPacket));
    unsigned long airTime = millis() - startTime;
    
    // Actualizar estadísticas
    stats.totalAirTime += airTime;
    
    if (state == RADIOLIB_ERR_NONE) {
        stats.packetsSent++;
        
        // SOLO mostrar debug en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] Packet enviado exitosamente");
            Serial.println("[LoRa] PacketID: " + String(packet.packetID) + ", Air time: " + String(airTime) + " ms");
        }
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return true;
    } else {
        stats.packetsLost++;
        
        // SOLO mostrar error en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Fallo en transmisión");
            Serial.println("[LoRa] Error code: " + String(state));
        }
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return false;
    }
}

/*
 * RECEPCIÓN Y PROCESAMIENTO DE PACKETS - ACTUALIZADO
 */
bool LoRaManager::receivePacket(LoRaPacket* packet) {
    if (!packet) return false;
    
    // Recibir datos del radio
    int state = radio.readData((uint8_t*)packet, sizeof(LoRaPacket));
    
    if (state == RADIOLIB_ERR_NONE) {
        // Obtener estadísticas de señal
        stats.lastRSSI = radio.getRSSI();
        stats.lastSNR = radio.getSNR();
        
        // Validar packet básico
        if (!validatePacket(packet)) {
            stats.packetsLost++;
            // SOLO mostrar en modo ADMIN
            if (configManager.isAdminMode()) {
                Serial.println("[LoRa] Packet inválido (checksum)");
            }
            return false;
        }
        
        // Verificar duplicados
        if (shouldFilterReceived(packet)) {
            stats.duplicatesIgnored++;
            // SOLO mostrar en modo ADMIN
            if (configManager.isAdminMode()) {
                Serial.println("[LoRa] Packet duplicado ignorado (sourceID=" + String(packet->sourceID) + ", packetID=" + String(packet->packetID) + ")");
            }
            return false;  // Packet duplicado, ignorar
        }
        
        // Packet válido y nuevo
        stats.packetsReceived++;
        
        // SOLO mostrar debug en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] Packet válido recibido");
            Serial.println("[LoRa] RSSI: " + String(stats.lastRSSI) + " dBm");
            Serial.println("[LoRa] SNR: " + String(stats.lastSNR) + " dB");
            Serial.println("[LoRa] Source: " + String(packet->sourceID) + ", Hops: " + String(packet->hops) + "/" + String(packet->maxHops));
            Serial.println("[LoRa] Message Type: " + String(packet->messageType));
        }
        
        // Agregar a seen packets para evitar futuras retransmisiones
        addToRecentPackets(packet->sourceID, packet->packetID);
        
        // IMPORTANTE: Procesar contenido ANTES del retransmit
        switch (packet->messageType) {
            case MSG_GPS_DATA:
                // Procesar datos GPS recibidos
                float lat, lon;
                uint32_t timestamp;
                uint16_t sourceID;
                if (processGPSPacket(packet, &lat, &lon, &timestamp, &sourceID)) {
                    // SOLO mostrar debug en modo ADMIN
                    if (configManager.isAdminMode()) {
                        Serial.println("[LoRa] GPS recibido de device " + String(sourceID) + 
                                     ": " + String(lat, 6) + "," + String(lon, 6));
                    }
                }
                break;
                
            case MSG_DISCOVERY_REQUEST:
                // NUEVO: Procesar solicitud de discovery
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Discovery request recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processDiscoveryRequest(packet);
                break;
                
            case MSG_DISCOVERY_RESPONSE:
                // NUEVO: Procesar respuesta de discovery
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Discovery response recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processDiscoveryResponse(packet);
                break;
                
            case MSG_CONFIG_CMD:
                // NUEVO: Procesar comando de configuración remota
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Comando de configuración recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processRemoteConfigCommand(packet);
                break;
                
            case MSG_CONFIG_RESPONSE:
                // NUEVO: Procesar respuesta de configuración
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Respuesta de configuración recibida de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processRemoteConfigResponse(packet);
                break;
                
            case MSG_HEARTBEAT:
                // SOLO mostrar en modo ADMIN
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Heartbeat recibido de device " + String(packet->sourceID));
                }
                break;
                
            default:
                // SOLO mostrar en modo ADMIN
                if (configManager.isAdminMode()) {
                    Serial.println("[LoRa] Packet tipo desconocido: " + String(packet->messageType));
                }
                break;
        }
        
        // Verificar si debe retransmitirse (solo para ciertos tipos de mensaje)
        bool shouldRetransmit = false;
        if (packet->messageType == MSG_GPS_DATA || packet->messageType == MSG_CONFIG_CMD || 
            packet->messageType == MSG_DISCOVERY_REQUEST) {
            shouldRetransmit = true;
        }

        // DEBUG: Verificar por qué no se retransmite
        if (configManager.isAdminMode())
        {
            Serial.println("[DEBUG] Message type: " + String(packet->messageType));
            Serial.println("[DEBUG] Should retransmit: " + String(shouldRetransmit));
        }

        if (shouldRetransmit)
        {
            if (configManager.isAdminMode())
            {
                Serial.println("[DEBUG] Llamando perhapsRebroadcast()...");
            }
            if (perhapsRebroadcast(packet))
            {
                // ...
            }
            else
            {
                if (configManager.isAdminMode())
                {
                    Serial.println("[DEBUG] perhapsRebroadcast() devolvió false");
                }
            }
        }
        else
        {
            if (configManager.isAdminMode())
            {
                Serial.println("[DEBUG] Message type no válido para retransmisión");
            }
        }

        return true;
        
    } else {
        // SOLO mostrar error en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Fallo en recepción");
            Serial.println("[LoRa] Error code: " + String(state));
        }
        return false;
    }
}

/*
 * VERIFICAR SI HAY PACKETS DISPONIBLES (sin cambios)
 */
bool LoRaManager::isPacketAvailable() {
    // Verificar flag de recepción de RadioLib
    return radio.getIrqStatus() & RADIOLIB_SX126X_IRQ_RX_DONE;
}

/*
 * PROCESAR PACKET GPS RECIBIDO (sin cambios)
 */
bool LoRaManager::processGPSPacket(const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID) {
    if (!packet || packet->messageType != MSG_GPS_DATA) return false;
    
    // Extraer payload GPS
    GPSPayload* gpsPayload = (GPSPayload*)packet->payload;
    
    // Convertir datos
    payloadToGpsData(gpsPayload, lat, lon, timestamp);
    *sourceID = packet->sourceID;
    
    return true;
}

/*
 * LOOP PRINCIPAL DE ACTUALIZACIÓN (sin cambios)
 */
void LoRaManager::update() {
    // Limpiar packets antiguos cada 30 segundos
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup >= 30000) {
        cleanOldPackets();
        lastCleanup = millis();
        
        // SOLO mostrar en modo ADMIN
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] Packets en memoria: " + String(recentBroadcasts.size()));
        }
    }
    
    // Verificar si hay packets recibidos
    if (isPacketAvailable()) {
        LoRaPacket packet;
        if (receivePacket(&packet)) {
            // El procesamiento ya se hace en receivePacket()
        }
    }
}