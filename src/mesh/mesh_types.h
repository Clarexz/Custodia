/*
 * MESH_TYPES.H - Estructuras Compartidas para Sistema Mesh
 * 
 * Este archivo define todas las estructuras de datos, enums y constantes
 * compartidas entre los diferentes componentes del sistema mesh.
 * 
 */

#ifndef MESH_TYPES_H
#define MESH_TYPES_H

#include <Arduino.h>
#include "config.h"  // Para DeviceRole

/*
 * PROTOCOLO DE PACKETS MESH
 */

// Tipos de mensaje en la red mesh
enum LoRaMessageType {
    MSG_GPS_DATA = 0x01,        // Datos GPS de TRACKER
    MSG_MESH_ROUTE = 0x02,      // Mensaje de routing mesh
    MSG_CONFIG_CMD = 0x03,      // Comando de configuración remota
    MSG_HEARTBEAT = 0x04,       // Heartbeat para mantener conectividad
    MSG_ACK = 0x05              // Acknowledgment
};

// Estructura del packet LoRa optimizada
struct LoRaPacket {
    uint8_t messageType;        // Tipo de mensaje (LoRaMessageType)
    uint16_t sourceID;          // ID del dispositivo origen
    uint16_t destinationID;     // ID destino (0xFFFF = broadcast)
    uint8_t hops;               // Número de saltos realizados
    uint8_t maxHops;            // Máximo número de saltos permitidos
    uint32_t packetID;          // ID único del packet (evitar duplicados)
    uint8_t payloadLength;      // Longitud del payload en bytes
    uint8_t payload[32];        // Datos del mensaje (máximo 32 bytes)
    uint16_t checksum;          // Checksum para validación
} __attribute__((packed));

// Estructura específica para datos GPS (payload del packet)
struct GPSPayload {
    float latitude;             // 4 bytes
    float longitude;            // 4 bytes  
    uint32_t timestamp;         // 4 bytes
    uint16_t batteryVoltage;    // 2 bytes (mV) - Futuro
    uint8_t satellites;         // 1 byte - Info adicional
    uint8_t reserved;           // 1 byte - Para alineación
} __attribute__((packed));     // Total: 16 bytes

/*
 * MESHTASTIC ALGORITHM STRUCTURES
 */

// Duplicate packet detection record
struct PacketRecord {
    uint16_t sourceID;          // ID del nodo origen
    uint32_t packetID;          // ID único del packet
    unsigned long timestamp;    // Cuándo se vio este packet
};

// Contention Window values - Sacado de Meshtastic RadioInterface.cpp
struct ContentionWindow {
    static const uint8_t CWmin = 2;           // Ventana mínima
    static const uint8_t CWmax = 8;           // Ventana máxima
    static const uint16_t slotTimeMsec = 10;  // 10ms por slot
    
    // SNR Range - Sacados de Meshtastic
    static const int32_t SNR_MIN = -20;       // dB
    static const int32_t SNR_MAX = 15;        // dB
};

// Estadísticas de mesh networking
struct MeshStats {
    uint32_t duplicatesIgnored; // Packets duplicados ignorados
    uint32_t rebroadcasts;      // Packets retransmitidos
    uint32_t hopLimitReached;   // Packets descartados por hop limit
    uint32_t packetsMemory;     // Packets en memoria actualmente
};

/*
 * ESTADOS Y CONFIGURACIONES
 */

enum LoRaStatus {
    LORA_STATUS_INIT = 0,       // Inicializando
    LORA_STATUS_READY = 1,      // Listo para transmitir/recibir
    LORA_STATUS_TRANSMITTING = 2, // Transmitiendo
    LORA_STATUS_RECEIVING = 3,  // Modo recepción
    LORA_STATUS_ERROR = 4       // Error en el módulo
};

struct LoRaStats {
    uint32_t packetsSent;       // Packets enviados
    uint32_t packetsReceived;   // Packets recibidos válidos
    uint32_t packetsLost;       // Packets perdidos/corruptos
    float lastRSSI;             // Último RSSI recibido (dBm)
    float lastSNR;              // Último SNR recibido (dB)
    uint32_t totalAirTime;      // Tiempo total en el aire (ms)
    
    // Estadísticas mesh integradas
    MeshStats mesh;
};

/*
 * CONSTANTES DEL SISTEMA
 */

// Direcciones especiales
#define LORA_BROADCAST_ADDR     0xFFFF  // Dirección de broadcast
#define LORA_INVALID_ADDR       0x0000  // Dirección inválida

// Tamaños máximos
#define LORA_MAX_PAYLOAD_SIZE   32      // bytes - Máximo payload por packet
#define LORA_MAX_PACKET_SIZE    64      // bytes - Máximo tamaño total de packet

// Constantes Meshtastic
#define MESHTASTIC_MAX_HOPS     3       // Máximo saltos por defecto
#define MESHTASTIC_PACKET_ID_INVALID 0  // ID inválido

// Configuración de memoria para duplicate detection
#define MAX_RECENT_PACKETS      100     // Máximo packets en memoria
#define PACKET_MEMORY_TIME      300000  // 5 minutos en milisegundos

/*
 * CALLBACKS Y HANDLERS
 */

// Callback para cuando se recibe un packet GPS
typedef void (*GPSPacketCallback)(uint16_t sourceID, float lat, float lon, uint32_t timestamp);

// Callback para estadísticas de mesh
typedef void (*MeshStatsCallback)(const MeshStats& stats);

// Callback para eventos de mesh
typedef void (*MeshEventCallback)(const char* event, uint16_t nodeID);

/*
 * UTILIDADES INLINE
 */

// Verificar si una dirección es broadcast
inline bool isBroadcastAddress(uint16_t addr) {
    return (addr == LORA_BROADCAST_ADDR);
}

// Verificar si un packet ID es válido
inline bool isValidPacketID(uint32_t packetID) {
    return (packetID != MESHTASTIC_PACKET_ID_INVALID);
}

// Obtener string de tipo de mensaje
inline const char* getMessageTypeString(LoRaMessageType type) {
    switch (type) {
        case MSG_GPS_DATA: return "GPS_DATA";
        case MSG_MESH_ROUTE: return "MESH_ROUTE";
        case MSG_CONFIG_CMD: return "CONFIG_CMD";
        case MSG_HEARTBEAT: return "HEARTBEAT";
        case MSG_ACK: return "ACK";
        default: return "UNKNOWN";
    }
}

// Obtener string de rol para mesh priority
inline const char* getRolePriorityString(DeviceRole role) {
    switch (role) {
        case ROLE_REPEATER: return "HIGH (ROUTER)";
        case ROLE_TRACKER: return "NORMAL (CLIENT)";
        case ROLE_RECEIVER: return "NORMAL (CLIENT)";
        default: return "UNDEFINED";
    }
}

#endif // MESH_TYPES_H