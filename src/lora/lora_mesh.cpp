/*
 * LORA_MESH.CPP - Algoritmo Meshtastic Completo
 * 
 * Este archivo contiene toda la implementación del algoritmo
 * de Managed Flood Routing de Meshtastic: duplicate detection,
 * SNR-based delays y rebroadcast logic.
 * 
 * CORREGIDO: Todos los mensajes DEBUG y LoRa solo aparecen en modo ADMIN
 */

#include "../lora.h"
#include <algorithm>

/*
 * MESHTASTIC ALGORITHM: DUPLICATE DETECTION
 * Copiado exacto de FloodingRouter.cpp
 */
bool LoRaManager::wasSeenRecently(const LoRaPacket* packet) {
    // Buscar si ya vimos este packet (sourceID + packetID)
    for (const auto& record : recentBroadcasts) {
        if (record.sourceID == packet->sourceID && record.packetID == packet->packetID) {
            return true;  // Ya lo vimos
        }
    }
    return false;  // Packet nuevo
}

void LoRaManager::addToRecentPackets(uint16_t sourceID, uint32_t packetID) {
    // Si ya tenemos demasiados, eliminar el más antiguo
    if (recentBroadcasts.size() >= MAX_RECENT_PACKETS) {
        recentBroadcasts.erase(recentBroadcasts.begin());
    }
    
    // Agregar nuevo record
    PacketRecord record;
    record.sourceID = sourceID;
    record.packetID = packetID;
    record.timestamp = millis();
    
    recentBroadcasts.push_back(record);
}

void LoRaManager::cleanOldPackets() {
    unsigned long currentTime = millis();
    
    // Eliminar packets más antiguos que PACKET_MEMORY_TIME
    recentBroadcasts.erase(
        std::remove_if(recentBroadcasts.begin(), recentBroadcasts.end(),
            [currentTime](const PacketRecord& record) {
                return (currentTime - record.timestamp) > PACKET_MEMORY_TIME;
            }),
        recentBroadcasts.end()
    );
    
    // Debug info - SOLO EN MODO ADMIN
    if (configManager.isAdminMode() && recentBroadcasts.size() > 0) {
        Serial.println("[LoRa] Packets en memoria: " + String(recentBroadcasts.size()));
    }
}

/*
 * MESHTASTIC ALGORITHM: SNR-BASED DELAYS
 * Copiado exacto de RadioInterface.cpp
 */
uint8_t LoRaManager::getCWsize(float snr) {
    // Mapear SNR al tamaño de contention window
    // SNR bajo = CW pequeño (delay corto, prioridad alta)
    // SNR alto = CW grande (delay largo, prioridad baja)
    return map(snr, ContentionWindow::SNR_MIN, ContentionWindow::SNR_MAX, 
               ContentionWindow::CWmin, ContentionWindow::CWmax);
}

uint32_t LoRaManager::getTxDelayMsecWeighted(float snr, DeviceRole role) {
    uint8_t CWsize = getCWsize(snr);
    uint32_t delay = 0;
    
    // EXACT LOGIC FROM MESHTASTIC RadioInterface.cpp
    if (role == ROLE_REPEATER) {  // Como ROUTER en Meshtastic
        // ROUTERS/REPEATERS tienen MENOS delay (mayor prioridad)
        delay = random(0, pow(2, CWsize)) * ContentionWindow::slotTimeMsec;
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] REPEATER delay: " + String(delay) + " ms");
        }
    } else {
        // CLIENTS (TRACKER/RECEIVER) tienen MÁS delay
        delay = (2 * ContentionWindow::CWmax * ContentionWindow::slotTimeMsec) + 
                random(0, pow(2, CWsize)) * ContentionWindow::slotTimeMsec;
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] CLIENT delay: " + String(delay) + " ms");
        }
    }
    
    return delay;
}

uint32_t LoRaManager::getRandomDelay(uint8_t cwSize) {
    return random(0, pow(2, cwSize)) * ContentionWindow::slotTimeMsec;
}

/*
 * MESHTASTIC ALGORITHM: MESH LOGIC
 * Basado en FloodingRouter.cpp
 */
bool LoRaManager::shouldFilterReceived(const LoRaPacket* packet) {
    // Implementación de shouldFilterReceived de Meshtastic
    if (wasSeenRecently(packet)) {
        return true;  // Filtrar duplicado
    }
    return false;  // Procesar packet
}

bool LoRaManager::isRebroadcaster() {
    // Basado en FloodingRouter::isRebroadcaster()
    // Todos los roles excepto CLIENT_MUTE pueden retransmitir
    // En nuestro caso, todos los roles retransmiten
    return (currentRole != ROLE_NONE);
}

bool LoRaManager::isToUs(const LoRaPacket* packet) {
    // FIXED: Solo es "para nosotros" si es específicamente nuestro ID
    // Los broadcasts NO son "para nosotros" - deben retransmitirse
    bool result = (packet->destinationID == deviceID);
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] isToUs(): destID=" + String(packet->destinationID) + ", ourID=" + String(deviceID) + " → " + String(result));
    }
    return result;
}

bool LoRaManager::isFromUs(const LoRaPacket* packet) {
    // Verificar si el packet es de nosotros
    bool result = (packet->sourceID == deviceID);
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] isFromUs(): srcID=" + String(packet->sourceID) + ", ourID=" + String(deviceID) + " → " + String(result));
    }
    return result;
}

bool LoRaManager::isBroadcast(uint16_t destinationID) {
    return (destinationID == LORA_BROADCAST_ADDR);
}

bool LoRaManager::hasRolePriority(DeviceRole role) {
    // REPEATERS tienen prioridad (como ROUTERS en Meshtastic)
    return (role == ROLE_REPEATER);
}

/*
 * MESHTASTIC ALGORITHM: ENHANCED REBROADCAST
 * Basado exactamente en FloodingRouter::perhapsRebroadcast()
 */
bool LoRaManager::perhapsRebroadcast(const LoRaPacket* packet) {
    // DEBUG: Verificar entrada - SOLO EN MODO ADMIN
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] perhapsRebroadcast() ENTRADA");
        //Serial.println("[DEBUG] packetID: " + String(packet->packetID));
        //Serial.println("[DEBUG] sourceID: " + String(packet->sourceID) + ", ourID: " + String(deviceID));
        //Serial.println("[DEBUG] destinationID: " + String(packet->destinationID));
    }
    
    // No retransmitir si es para nosotros, o es de nosotros, o hop limit agotado
    bool toUs = isToUs(packet);
    bool fromUs = isFromUs(packet);
    bool hopLimitReached = (packet->hops >= packet->maxHops);
    
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] toUs: " + String(toUs) + ", fromUs: " + String(fromUs) + ", hopLimit: " + String(hopLimitReached));
    }
    
    if (toUs || fromUs || hopLimitReached) {
        if (hopLimitReached) {
            stats.hopLimitReached++;
            if (configManager.isAdminMode()) {
                Serial.println("[LoRa] Packet descartado: hop limit alcanzado (" + String(packet->hops) + "/" + String(packet->maxHops) + ")");
            }
        }
        if (configManager.isAdminMode()) {
            if (toUs) {
                //Serial.println("[DEBUG] SALIENDO: packet es para nosotros");
            }
            if (fromUs) {
                //Serial.println("[DEBUG] SALIENDO: packet es de nosotros");
            }
        }
        return false;
    }
    
    // Verificar que packet ID sea válido
    if (packet->packetID == MESHTASTIC_PACKET_ID_INVALID) {
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] Packet ignorado: ID inválido");
        }
        return false;
    }
    
    // Verificar si somos rebroadcaster
    bool canRebroadcast = isRebroadcaster();
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] isRebroadcaster: " + String(canRebroadcast));
    }
    
    if (!canRebroadcast) {
        if (configManager.isAdminMode()) {
            //Serial.println("[LoRa] No retransmitir: Role no permite rebroadcast");
        }
        return false;
    }
    
    if (configManager.isAdminMode()) {
        //Serial.println("[DEBUG] ¡TODOS LOS CHECKS PASARON! Procediendo con retransmisión...");
    }
    
    // Calcular delay basado en SNR y role
    uint32_t meshDelay = getTxDelayMsecWeighted(stats.lastSNR, currentRole);
    
    if (configManager.isAdminMode()) {
        Serial.println("[LoRa] Programando retransmisión en " + String(meshDelay) + " ms");
        Serial.println("[LoRa] SNR: " + String(stats.lastSNR) + " dB, Role: " + String(currentRole));
    }
    
    // Delay antes de retransmitir (SNR-based)
    delay(meshDelay);
    
    // Crear copia del packet para retransmisión
    LoRaPacket retransmitPacket = *packet;
    retransmitPacket.hops++;  // Incrementar hop count
    
    // Recalcular checksum
    retransmitPacket.checksum = calculateChecksum(&retransmitPacket);
    
    // Cambiar a modo transmisión
    LoRaStatus previousStatus = status;
    status = LORA_STATUS_TRANSMITTING;
    
    // Retransmitir packet
    unsigned long startTime = millis();
    int state = radio.transmit((uint8_t*)&retransmitPacket, sizeof(LoRaPacket));
    unsigned long airTime = millis() - startTime;
    
    stats.totalAirTime += airTime;
    
    if (state == RADIOLIB_ERR_NONE) {
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] Retransmisión exitosa (hop " + String(retransmitPacket.hops) + ")");
            Serial.println("[LoRa] Air time: " + String(airTime) + " ms");
        }
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return true;
    } else {
        stats.packetsLost++;
        if (configManager.isAdminMode()) {
            Serial.println("[LoRa] ERROR: Fallo en retransmisión");
            Serial.println("[LoRa] Error code: " + String(state));
        }
        
        // Volver a modo recepción
        radio.startReceive();
        status = previousStatus;
        return false;
    }
}