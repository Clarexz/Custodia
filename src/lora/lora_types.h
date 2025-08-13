/*
 * LORA_TYPES.H - Tipos y Estructuras del Sistema LoRa (VERSIÓN LIMPIA)
 * 
 * CORREGIDO: Sin DeviceRole - se incluye desde config_manager.h
 */

#ifndef LORA_TYPES_H
#define LORA_TYPES_H

#include <Arduino.h>
#include <vector>

/*
 * NOTA: DeviceRole se define en config_manager.h
 * No se incluye aquí para evitar dependencias circulares
 */

/*
 * TIPOS DE MENSAJE LORA
 */

enum LoRaMessageType {
    MSG_GPS_DATA = 0x01,
    MSG_MESH_ROUTE = 0x02,
    MSG_CONFIG_CMD = 0x03,
    MSG_CONFIG_RESPONSE = 0x04,
    MSG_DISCOVERY_REQUEST = 0x05,
    MSG_DISCOVERY_RESPONSE = 0x06,
    MSG_HEARTBEAT = 0x07,
    MSG_ACK = 0x08
};

/*
 * ESTRUCTURAS DE PACKETS
 */

struct LoRaPacket {
    uint8_t messageType;
    uint16_t sourceID;
    uint16_t destinationID;
    uint8_t hops;
    uint8_t maxHops;
    uint32_t packetID;
    uint32_t networkHash;
    uint8_t payloadLength;
    uint8_t payload[32];
    uint16_t checksum;
} __attribute__((packed));

struct GPSPayload {
    float latitude;
    float longitude;
    uint32_t timestamp;
    uint16_t batteryVoltage;
    uint8_t satellites;
    uint8_t reserved;
} __attribute__((packed));

/*
 * CONFIGURACIÓN REMOTA
 */

enum RemoteCommandType {
    REMOTE_CMD_GPS_INTERVAL = 0x01,
    REMOTE_CMD_DATA_MODE = 0x02,
    REMOTE_CMD_STATUS = 0x03,
    REMOTE_CMD_REBOOT = 0x04
};

struct RemoteConfigCmd {
    uint8_t commandType;
    uint32_t value;
    uint32_t sequenceID;
    uint8_t reserved[3];
} __attribute__((packed));

struct RemoteConfigResponse {
    uint8_t commandType;
    uint8_t success;
    uint32_t sequenceID;
    uint32_t currentValue;
    char message[16];
} __attribute__((packed));

struct DiscoveryInfo {
    uint8_t role;
    uint16_t gpsInterval;
    uint8_t dataMode;
    uint8_t region;
    uint16_t batteryVoltage;
    uint32_t uptime;
    uint8_t reserved[4];
} __attribute__((packed));

/*
 * MESHTASTIC COMPONENTS
 */

struct PacketRecord {
    uint16_t sourceID;
    uint32_t packetID;
    unsigned long timestamp;
};

struct ContentionWindow {
    static const uint8_t CWmin = 2;
    static const uint8_t CWmax = 8;
    static const uint16_t slotTimeMsec = 10;
    static const int32_t SNR_MIN = -20;
    static const int32_t SNR_MAX = 15;
};

/*
 * ESTADOS LORA
 */

enum LoRaStatus {
    LORA_STATUS_INIT = 0,
    LORA_STATUS_READY = 1,
    LORA_STATUS_TRANSMITTING = 2,
    LORA_STATUS_RECEIVING = 3,
    LORA_STATUS_ERROR = 4
};

struct LoRaStats {
    uint32_t packetsSent;
    uint32_t packetsReceived;
    uint32_t packetsLost;
    float lastRSSI;
    float lastSNR;
    uint32_t totalAirTime;
    uint32_t duplicatesIgnored;
    uint32_t rebroadcasts;
    uint32_t hopLimitReached;
    uint32_t networkFilteredPackets;
};

/*
 * CONSTANTES
 */

#define LORA_TX_TIMEOUT         5000
#define LORA_RX_TIMEOUT         1000
#define LORA_INIT_TIMEOUT       10000
#define LORA_BROADCAST_ADDR     0xFFFF
#define LORA_INVALID_ADDR       0x0000
#define LORA_MAX_PAYLOAD_SIZE   32
#define LORA_MAX_PACKET_SIZE    64
#define MESHTASTIC_MAX_HOPS     3
#define MESHTASTIC_PACKET_ID_INVALID 0
#define REMOTE_CONFIG_TIMEOUT   5000
#define DISCOVERY_TIMEOUT       3000
#define MAX_RECENT_PACKETS      100
#define PACKET_MEMORY_TIME      300000

#endif