/*
 * PACKET_MANAGER.H - Gestión de Packets y Duplicate Detection
 * 
 * Este módulo maneja la detección de packets duplicados y la gestión
 * de memoria de packets recientes, implementando la funcionalidad
 * wasSeenRecently() de Meshtastic FloodingRouter.cpp
 */

#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include <vector>
#include "mesh_types.h"

/*
 * CLASE PRINCIPAL - PacketManager
 * 
 * Responsable de:
 * - Detección de packets duplicados
 * - Gestión de memoria de packets recientes
 * - Limpieza automática de packets antiguos
 * - Estadísticas de duplicate detection
 */
class PacketManager {
private:
    // Vector para almacenar packets recientes (sacado de Meshtastic)
    std::vector<PacketRecord> recentPackets;
    
    // Estadísticas internas
    uint32_t totalPacketsSeen;
    uint32_t duplicatesDetected;
    unsigned long lastCleanup;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    PacketManager();
    ~PacketManager();
    
    /*
     * MÉTODOS PRINCIPALES - Duplicate Detection
     */
    
    // Verificar si un packet ya fue visto recientemente
    // Implementación de Meshtastic FloodingRouter::wasSeenRecently()
    bool wasSeenRecently(const LoRaPacket* packet);
    bool wasSeenRecently(uint16_t sourceID, uint32_t packetID);
    
    // Agregar packet a la lista de recientes
    void addToRecentPackets(const LoRaPacket* packet);
    void addToRecentPackets(uint16_t sourceID, uint32_t packetID);
    
    // Limpiar packets antiguos de la memoria
    void cleanOldPackets();
    void cleanOldPackets(unsigned long maxAge);
    
    /*
     * GESTIÓN DE MEMORIA
     */
    
    // Obtener número de packets en memoria
    uint16_t getPacketsInMemory() const;
    
    // Verificar si la memoria está llena
    bool isMemoryFull() const;
    
    // Forzar limpieza completa
    void clearAllPackets();
    
    // Configurar tamaño máximo de memoria
    void setMaxPackets(uint16_t maxPackets);
    
    /*
     * ESTADÍSTICAS Y DIAGNÓSTICO
     */
    
    // Obtener estadísticas de duplicate detection
    uint32_t getTotalPacketsSeen() const { return totalPacketsSeen; }
    uint32_t getDuplicatesDetected() const { return duplicatesDetected; }
    float getDuplicateRate() const;
    
    // Información de memoria
    uint16_t getMemoryUsage() const;
    unsigned long getOldestPacketAge() const;
    
    // Debug y diagnóstico
    void printStatistics() const;
    void printMemoryInfo() const;
    void printRecentPackets() const;
    
    /*
     * CONFIGURACIÓN
     */
    
    // Configurar tiempo máximo de memoria de packets
    void setPacketMemoryTime(unsigned long memoryTimeMs);
    
    // Configurar intervalo de limpieza automática
    void setCleanupInterval(unsigned long intervalMs);
    
    // Habilitar/deshabilitar limpieza automática
    void setAutoCleanup(bool enabled);
    
    /*
     * ACTUALIZACIÓN PERIÓDICA
     */
    
    // Método que debe llamarse periódicamente para mantener la memoria
    void update();
    
private:
    /*
     * MÉTODOS PRIVADOS
     */
    
    // Configuración interna
    uint16_t maxRecentPackets;
    unsigned long packetMemoryTime;
    unsigned long cleanupInterval;
    bool autoCleanupEnabled;
    
    // Buscar packet en la lista de recientes
    bool findPacketRecord(uint16_t sourceID, uint32_t packetID) const;
    
    // Eliminar packet más antiguo si la memoria está llena
    void removeOldestPacket();
    
    // Verificar si un packet record está vencido
    bool isPacketExpired(const PacketRecord& record, unsigned long currentTime) const;
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en packet_manager.cpp
 */
extern PacketManager packetManager;

/*
 * UTILIDADES HELPER
 */

// Crear un PacketRecord desde un LoRaPacket
PacketRecord createPacketRecord(const LoRaPacket* packet);
PacketRecord createPacketRecord(uint16_t sourceID, uint32_t packetID);

// Comparar dos PacketRecords
bool arePacketRecordsEqual(const PacketRecord& a, const PacketRecord& b);

// Obtener edad de un PacketRecord en milisegundos
unsigned long getPacketRecordAge(const PacketRecord& record);

#endif // PACKET_MANAGER_H