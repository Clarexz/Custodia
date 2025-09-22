/*
 * LORA_COMM.CPP - Transmisión y Recepción Básica
 * 
 * ACTUALIZADO: Agregado manejo de mensajes de configuración remota y discovery
 */

#include "../lora.h"
#include "../gps/gps_manager.h"

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
    packet.networkHash = configManager.getActiveNetworkHash();
    
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

bool LoRaManager::isPacketFromSameNetwork(const LoRaPacket* packet) {
    // Si no hay network activa, aceptar todos los packets (modo legacy)
    if (!configManager.hasActiveNetwork()) {
        return true;
    }
    
    // Comparar hash del packet con hash de la network activa
    return (packet->networkHash == configManager.getActiveNetworkHash());
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

        if (!isPacketFromSameNetwork(packet)) {
            // Incrementar estadística de filtrado
            stats.networkFilteredPackets++;
            
            // Log opcional para debugging en modo ADMIN
            if (configManager.isAdminMode()) {
                Serial.printf("[NETWORK] Packet filtrado - Hash recibido: %08X vs activo: %08X\n", 
                             packet->networkHash, configManager.getActiveNetworkHash());
            }
            
            return false; // Rechazar packet de network diferente
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
        
        bool adminMode = configManager.isAdminMode();
        float receivedLat = 0.0f;
        float receivedLon = 0.0f;
        uint32_t receivedTimestamp = 0;
        float receivedVoltage = 0.0f;
        bool hasGPSDetails = false;

        if (adminMode) {
            String roleName = configManager.getRoleString(currentRole);
            switch (currentRole) {
                case ROLE_REPEATER:
                    roleName += " (ROUTER priority)";
                    break;
                case ROLE_TRACKER:
                case ROLE_RECEIVER:
                    roleName += " (CLIENT priority)";
                    break;
                default:
                    break;
            }

            Serial.println("============== STATUS ==============");
            Serial.println("Role: " + roleName);
            Serial.println("Estado LoRa: " + getStatusString());

            if (configManager.hasActiveNetwork()) {
                SimpleNetwork* network = configManager.getActiveNetwork();
                if (network) {
                    String hashStr = String(network->hash, HEX);
                    hashStr.toLowerCase();
                    Serial.println("Network: " + network->name + " (Hash: " + hashStr + ")");
                } else {
                    Serial.println("Network: NINGUNA ACTIVA - Modo legacy");
                }
            } else {
                Serial.println("Network: NINGUNA ACTIVA - Modo legacy");
            }

            Serial.println("Posición propia: " + gpsManager.formatCoordinates());
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
                    GPSPayload* gpsPayload = (GPSPayload*)packet->payload;
                    String sourceStr = String(sourceID);
                    while (sourceStr.length() < 3) {
                        sourceStr = "0" + sourceStr;
                    }
                    receivedLat = lat;
                    receivedLon = lon;
                    receivedTimestamp = timestamp;
                    receivedVoltage = gpsPayload->batteryVoltage;
                    hasGPSDetails = true;
                    lastSimplePacket = sourceStr + "," +
                                       String(lat, 6) + "," +
                                       String(lon, 6) + "," +
                                       String(gpsPayload->batteryVoltage) + "," +
                                       String(timestamp);
                    simplePacketPending = true;
                }
                break;
                
            case MSG_DISCOVERY_REQUEST:
                // NUEVO: Procesar solicitud de discovery
                if (adminMode) {
                    Serial.println("[LoRa] Discovery request recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processDiscoveryRequest(packet);
                break;
                
            case MSG_DISCOVERY_RESPONSE:
                // NUEVO: Procesar respuesta de discovery
                if (adminMode) {
                    Serial.println("[LoRa] Discovery response recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processDiscoveryResponse(packet);
                break;
                
            case MSG_CONFIG_CMD:
                // NUEVO: Procesar comando de configuración remota
                if (adminMode) {
                    Serial.println("[LoRa] Comando de configuración recibido de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processRemoteConfigCommand(packet);
                break;
                
            case MSG_CONFIG_RESPONSE:
                // NUEVO: Procesar respuesta de configuración
                if (adminMode) {
                    Serial.println("[LoRa] Respuesta de configuración recibida de device " + String(packet->sourceID));
                }
                // Procesar inmediatamente
                processRemoteConfigResponse(packet);
                break;
                
            case MSG_HEARTBEAT:
                // SOLO mostrar en modo ADMIN
                if (adminMode) {
                    Serial.println("[LoRa] Heartbeat recibido de device " + String(packet->sourceID));
                }
                break;
                
            default:
                // SOLO mostrar en modo ADMIN
                if (adminMode) {
                    Serial.println("[LoRa] Packet tipo desconocido: " + String(packet->messageType));
                }
                break;
        }

        if (adminMode) {
            Serial.println("Packet válido recibido");
            Serial.println("RSSI: " + String(stats.lastRSSI) + " dBm");
            Serial.println("SNR: " + String(stats.lastSNR) + " dB");
            Serial.println("Source ID: " + String(packet->sourceID) + ", Hops: " + String(packet->hops) + "/" + String(packet->maxHops));
            if (hasGPSDetails) {
                Serial.println("Posición recibida: " + String(receivedLat, 6) + "," + String(receivedLon, 6));
                Serial.println("Timestamp: " + String(receivedTimestamp));
                Serial.println("voltaje: " + String(receivedVoltage, 2));
            }
        }

        // Verificar si debe retransmitirse (solo para ciertos tipos de mensaje)
        bool shouldRetransmit = false;
        if (packet->messageType == MSG_GPS_DATA || packet->messageType == MSG_CONFIG_CMD || 
            packet->messageType == MSG_DISCOVERY_REQUEST) {
            shouldRetransmit = true;
        }

        if (shouldRetransmit) {
            perhapsRebroadcast(packet);
        }

        if (adminMode) {
            Serial.println("Packets recibidos: " + String(stats.packetsReceived));
            Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
            Serial.println("Retransmisiones hechas: " + String(stats.rebroadcasts));
            Serial.println("Network filtrados: " + String(stats.networkFilteredPackets));
            Serial.println("Packets en memoria: " + String(recentBroadcasts.size()));
            Serial.println("=====================================");
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

bool LoRaManager::fetchSimplePacket(String& out) {
    if (!simplePacketPending) {
        return false;
    }
    out = lastSimplePacket;
    simplePacketPending = false;
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
    }
    
    // Verificar si hay packets recibidos
    if (isPacketAvailable()) {
        LoRaPacket packet;
        if (receivePacket(&packet)) {
            // El procesamiento ya se hace en receivePacket()
        }
    }
}
