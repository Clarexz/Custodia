/*
 * PACKET_MANAGER.CPP - Implementación de Gestión de Packets
 * 
 * Implementa la detección de packets duplicados usando el algoritmo
 * de Meshtastic FloodingRouter.cpp
 */

#include "packet_manager.h"
#include <algorithm>

/*
 * INSTANCIA GLOBAL
 */
PacketManager packetManager;

/*
 * CONSTRUCTOR
 */
PacketManager::PacketManager() {
    // Configuración por defecto (valores de Meshtastic)
    maxRecentPackets = MAX_RECENT_PACKETS;
    packetMemoryTime = PACKET_MEMORY_TIME;
    cleanupInterval = 30000;  // 30 segundos
    autoCleanupEnabled = true;
    
    // Estadísticas
    totalPacketsSeen = 0;
    duplicatesDetected = 0;
    lastCleanup = 0;
    
    // Reservar memoria para mejorar performance
    recentPackets.reserve(maxRecentPackets);
    
    Serial.println("[PacketManager] Inicializado con duplicate detection de Meshtastic");
}

/*
 * DESTRUCTOR
 */
PacketManager::~PacketManager() {
    clearAllPackets();
}

/*
 * DUPLICATE DETECTION - Implementación de Meshtastic
 */
bool PacketManager::wasSeenRecently(const LoRaPacket* packet) {
    if (!packet) return false;
    return wasSeenRecently(packet->sourceID, packet->packetID);
}

bool PacketManager::wasSeenRecently(uint16_t sourceID, uint32_t packetID) {
    // Incrementar contador total
    totalPacketsSeen++;
    
    // Buscar en packets recientes
    for (const auto& record : recentPackets) {
        if (record.sourceID == sourceID && record.packetID == packetID) {
            duplicatesDetected++;
            return true;  // Packet duplicado encontrado
        }
    }
    
    return false;  // Packet nuevo
}

/*
 * AGREGAR PACKETS A MEMORIA RECIENTE
 */
void PacketManager::addToRecentPackets(const LoRaPacket* packet) {
    if (!packet) return;
    addToRecentPackets(packet->sourceID, packet->packetID);
}

void PacketManager::addToRecentPackets(uint16_t sourceID, uint32_t packetID) {
    // Verificar que el packet ID sea válido
    if (!isValidPacketID(packetID)) {
        return;
    }
    
    // Si la memoria está llena, eliminar el más antiguo
    if (recentPackets.size() >= maxRecentPackets) {
        recentPackets.erase(recentPackets.begin());
    }
    
    // Crear nuevo record
    PacketRecord record;
    record.sourceID = sourceID;
    record.packetID = packetID;
    record.timestamp = millis();
    
    // Agregar al final del vector
    recentPackets.push_back(record);
}

/*
 * LIMPIEZA DE PACKETS ANTIGUOS
 */
void PacketManager::cleanOldPackets() {
    cleanOldPackets(packetMemoryTime);
}

void PacketManager::cleanOldPackets(unsigned long maxAge) {
    unsigned long currentTime = millis();
    
    // Eliminar packets más antiguos que maxAge
    recentPackets.erase(
        std::remove_if(recentPackets.begin(), recentPackets.end(),
            [currentTime, maxAge](const PacketRecord& record) {
                return (currentTime - record.timestamp) > maxAge;
            }),
        recentPackets.end()
    );
    
    lastCleanup = currentTime;
}

/*
 * GESTIÓN DE MEMORIA
 */
uint16_t PacketManager::getPacketsInMemory() const {
    return recentPackets.size();
}

bool PacketManager::isMemoryFull() const {
    return recentPackets.size() >= maxRecentPackets;
}

void PacketManager::clearAllPackets() {
    recentPackets.clear();
    Serial.println("[PacketManager] Memoria de packets limpiada");
}

void PacketManager::setMaxPackets(uint16_t maxPackets) {
    maxRecentPackets = maxPackets;
    recentPackets.reserve(maxPackets);
    
    // Si actualmente tenemos más packets, limpiar los más antiguos
    while (recentPackets.size() > maxPackets) {
        recentPackets.erase(recentPackets.begin());
    }
}

/*
 * ESTADÍSTICAS
 */
float PacketManager::getDuplicateRate() const {
    if (totalPacketsSeen == 0) return 0.0f;
    return (float)duplicatesDetected / (float)totalPacketsSeen * 100.0f;
}

uint16_t PacketManager::getMemoryUsage() const {
    if (maxRecentPackets == 0) return 0;
    return (recentPackets.size() * 100) / maxRecentPackets;
}

unsigned long PacketManager::getOldestPacketAge() const {
    if (recentPackets.empty()) return 0;
    
    unsigned long currentTime = millis();
    unsigned long oldestTime = currentTime;
    
    for (const auto& record : recentPackets) {
        if (record.timestamp < oldestTime) {
            oldestTime = record.timestamp;
        }
    }
    
    return currentTime - oldestTime;
}

/*
 * DEBUG Y DIAGNÓSTICO
 */
void PacketManager::printStatistics() const {
    Serial.println("\n[PacketManager] === ESTADÍSTICAS ===");
    Serial.println("Packets vistos total: " + String(totalPacketsSeen));
    Serial.println("Duplicados detectados: " + String(duplicatesDetected));
    Serial.println("Tasa de duplicados: " + String(getDuplicateRate(), 2) + "%");
    Serial.println("Packets en memoria: " + String(getPacketsInMemory()) + "/" + String(maxRecentPackets));
    Serial.println("Uso de memoria: " + String(getMemoryUsage()) + "%");
    Serial.println("Edad packet más antiguo: " + String(getOldestPacketAge()) + " ms");
    Serial.println("=====================================");
}

void PacketManager::printMemoryInfo() const {
    Serial.println("\n[PacketManager] === MEMORIA ===");
    Serial.println("Capacidad máxima: " + String(maxRecentPackets));
    Serial.println("Packets actuales: " + String(recentPackets.size()));
    Serial.println("Memoria libre: " + String(maxRecentPackets - recentPackets.size()));
    Serial.println("Tiempo de retención: " + String(packetMemoryTime / 1000) + " segundos");
    Serial.println("Auto-limpieza: " + String(autoCleanupEnabled ? "HABILITADA" : "DESHABILITADA"));
    Serial.println("========================");
}

void PacketManager::printRecentPackets() const {
    Serial.println("\n[PacketManager] === PACKETS RECIENTES ===");
    
    if (recentPackets.empty()) {
        Serial.println("No hay packets en memoria");
        Serial.println("==============================");
        return;
    }
    
    unsigned long currentTime = millis();
    
    for (size_t i = 0; i < recentPackets.size(); i++) {
        const auto& record = recentPackets[i];
        unsigned long age = currentTime - record.timestamp;
        
        Serial.println("Packet " + String(i + 1) + ": SourceID=" + String(record.sourceID) + 
                      ", PacketID=" + String(record.packetID) + 
                      ", Edad=" + String(age) + "ms");
    }
    Serial.println("==============================");
}

/*
 * CONFIGURACIÓN
 */
void PacketManager::setPacketMemoryTime(unsigned long memoryTimeMs) {
    packetMemoryTime = memoryTimeMs;
    Serial.println("[PacketManager] Tiempo de memoria cambiado a: " + String(memoryTimeMs / 1000) + " segundos");
}

void PacketManager::setCleanupInterval(unsigned long intervalMs) {
    cleanupInterval = intervalMs;
}

void PacketManager::setAutoCleanup(bool enabled) {
    autoCleanupEnabled = enabled;
    Serial.println("[PacketManager] Auto-limpieza: " + String(enabled ? "HABILITADA" : "DESHABILITADA"));
}

/*
 * ACTUALIZACIÓN PERIÓDICA
 */
void PacketManager::update() {
    // Limpieza automática si está habilitada
    if (autoCleanupEnabled) {
        unsigned long currentTime = millis();
        if (currentTime - lastCleanup >= cleanupInterval) {
            cleanOldPackets();
        }
    }
}

/*
 * MÉTODOS PRIVADOS
 */
bool PacketManager::findPacketRecord(uint16_t sourceID, uint32_t packetID) const {
    for (const auto& record : recentPackets) {
        if (record.sourceID == sourceID && record.packetID == packetID) {
            return true;
        }
    }
    return false;
}

void PacketManager::removeOldestPacket() {
    if (!recentPackets.empty()) {
        recentPackets.erase(recentPackets.begin());
    }
}

bool PacketManager::isPacketExpired(const PacketRecord& record, unsigned long currentTime) const {
    return (currentTime - record.timestamp) > packetMemoryTime;
}

/*
 * UTILIDADES HELPER
 */
PacketRecord createPacketRecord(const LoRaPacket* packet) {
    PacketRecord record;
    record.sourceID = packet->sourceID;
    record.packetID = packet->packetID;
    record.timestamp = millis();
    return record;
}

PacketRecord createPacketRecord(uint16_t sourceID, uint32_t packetID) {
    PacketRecord record;
    record.sourceID = sourceID;
    record.packetID = packetID;
    record.timestamp = millis();
    return record;
}

bool arePacketRecordsEqual(const PacketRecord& a, const PacketRecord& b) {
    return (a.sourceID == b.sourceID && a.packetID == b.packetID);
}

unsigned long getPacketRecordAge(const PacketRecord& record) {
    return millis() - record.timestamp;
}