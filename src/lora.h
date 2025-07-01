/*
 * LORA.H - Sistema LoRa Enhanced con Algoritmo Meshtastic
 * 
 * ACTUALIZADO: Agregado soporte para configuración remota y discovery
 */

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <RadioLib.h>
#include <vector>
#include "config.h"  // Para DeviceRole

/*
 * CONFIGURACIÓN DE HARDWARE PARA XIAO ESP32S3 + WIO SX1262
 */

// Configuración oficial de Meshtastic para XIAO S3 + SX1262
#define LORA_SCK_PIN    7   // LORA_SCK
#define LORA_MISO_PIN   8   // LORA_MISO
#define LORA_MOSI_PIN   9   // LORA_MOSI
#define LORA_NSS_PIN    41  // LORA_CS
#define LORA_DIO1_PIN   39  // LORA_DIO1
#define LORA_NRST_PIN   42  // LORA_RESET
#define LORA_BUSY_PIN   40  // SX126X_BUSY

/*
 * CONFIGURACIÓN DE RADIO
 */

// Configuración optimizada para velocidad vs alcance
#define LORA_BANDWIDTH      125.0f  // kHz - Balance velocidad/alcance
#define LORA_SPREADING_FACTOR   7   // SF7 = Rápido, SF12 = Largo alcance
#define LORA_CODING_RATE        5   // 4/5 = Buena corrección de errores
#define LORA_TX_POWER          14   // dBm - Potencia de transmisión
#define LORA_PREAMBLE_LENGTH    8   // Símbolos de preámbulo
#define LORA_SYNC_WORD     0x12     // Palabra de sincronización personalizada

/*
 * PROTOCOLO DE PACKETS - ACTUALIZADO
 */

// Tipos de mensaje en la red mesh
enum LoRaMessageType {
    MSG_GPS_DATA = 0x01,            // Datos GPS de TRACKER
    MSG_MESH_ROUTE = 0x02,          // Mensaje de routing mesh
    MSG_CONFIG_CMD = 0x03,          // NUEVO: Comando de configuración remota
    MSG_CONFIG_RESPONSE = 0x04,     // NUEVO: Respuesta a comando remoto
    MSG_DISCOVERY_REQUEST = 0x05,   // NUEVO: Solicitud de discovery
    MSG_DISCOVERY_RESPONSE = 0x06,  // NUEVO: Respuesta de discovery
    MSG_HEARTBEAT = 0x07,           // Heartbeat para mantener conectividad
    MSG_ACK = 0x08                  // Acknowledgment
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
    uint16_t batteryVoltage;    // 2 bytes (mV)
    uint8_t satellites;         // 1 byte - Info adicional
    uint8_t reserved;           // 1 byte - Para alineación
} __attribute__((packed));     // Total: 16 bytes

/*
 * NUEVAS ESTRUCTURAS PARA CONFIGURACIÓN REMOTA
 */

// Tipos de comandos remotos
enum RemoteCommandType {
    REMOTE_CMD_GPS_INTERVAL = 0x01,     // Cambiar intervalo GPS
    REMOTE_CMD_DATA_MODE = 0x02,        // Cambiar modo de datos
    REMOTE_CMD_STATUS = 0x03,           // Solicitar status
    REMOTE_CMD_REBOOT = 0x04            // Reiniciar dispositivo
};

// Comando de configuración remota (payload)
struct RemoteConfigCmd {
    uint8_t commandType;        // Tipo de comando (RemoteCommandType)
    uint32_t value;             // Valor del comando
    uint32_t sequenceID;        // ID de secuencia para tracking
    uint8_t reserved[3];        // Padding para alineación
} __attribute__((packed));     // Total: 12 bytes

// Respuesta a comando remoto (payload)
struct RemoteConfigResponse {
    uint8_t commandType;        // Comando al que responde
    uint8_t success;            // 1 = éxito, 0 = error
    uint32_t sequenceID;        // ID de secuencia original
    uint32_t currentValue;      // Valor actual después del cambio
    char message[16];           // Mensaje de respuesta
} __attribute__((packed));     // Total: 26 bytes

// Información de discovery (payload)
struct DiscoveryInfo {
    uint8_t role;               // DeviceRole
    uint16_t gpsInterval;       // Intervalo GPS actual
    uint8_t dataMode;           // DataDisplayMode actual
    uint8_t region;             // LoRaRegion actual
    uint16_t batteryVoltage;    // Voltaje de batería
    uint32_t uptime;            // Tiempo encendido en segundos
    uint8_t reserved[4];        // Padding
} __attribute__((packed));     // Total: 16 bytes

/*
 * MESHTASTIC ALGORITHM COMPONENTS (sin cambios)
 */

// Duplicate packet detection - Basado exactamente en Meshtastic
struct PacketRecord {
    uint16_t sourceID;          // ID del nodo origen
    uint32_t packetID;          // ID único del packet
    unsigned long timestamp;    // Cuándo se vio este packet
};

// Contention Window values - EXACTOS de Meshtastic RadioInterface.cpp
struct ContentionWindow {
    static const uint8_t CWmin = 2;           // Ventana mínima
    static const uint8_t CWmax = 8;           // Ventana máxima
    static const uint16_t slotTimeMsec = 10;  // 10ms por slot
    
    // SNR Range - EXACTOS de Meshtastic
    static const int32_t SNR_MIN = -20;       // dB
    static const int32_t SNR_MAX = 15;        // dB
};

/*
 * ESTADOS Y ESTADÍSTICAS DEL SISTEMA LORA (sin cambios)
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
    
    // Estadísticas mesh
    uint32_t duplicatesIgnored; // Packets duplicados ignorados
    uint32_t rebroadcasts;      // Packets retransmitidos
    uint32_t hopLimitReached;   // Packets descartados por hop limit
};

/*
 * CLASE PRINCIPAL - LoRaManager ENHANCED
 */
class LoRaManager {
private:
    // === COMPONENTES EXISTENTES ===
    
    // Instancia del módulo SX1262 usando RadioLib
    SX1262 radio;
    
    // Estado actual del sistema LoRa
    LoRaStatus status;
    
    // Estadísticas de comunicación
    LoRaStats stats;
    
    // ID de este dispositivo
    uint16_t deviceID;
    
    // Contador para generar IDs únicos de packets
    uint32_t packetCounter;
    
    // Buffer para recepción
    uint8_t receiveBuffer[256];
    
    // === COMPONENTES MESHTASTIC ===
    
    // Duplicate Detection System
    std::vector<PacketRecord> recentBroadcasts;
    static const uint16_t MAX_RECENT_PACKETS = 100;
    static const unsigned long PACKET_MEMORY_TIME = 300000; // 5 minutos
    
    // Role Management para priority
    DeviceRole currentRole;
    
    // Contention Window instance
    ContentionWindow cw;
    
    /*
     * MÉTODOS PRIVADOS EXISTENTES
     */
    
    // Inicialización del hardware SX1262
    bool initRadio();
    
    // Configuración de parámetros de radio
    bool configureRadio();
    
    // Cálculo de checksum para validación
    uint16_t calculateChecksum(const LoRaPacket* packet);
    
    // Validación de packet recibido
    bool validatePacket(const LoRaPacket* packet);
    
    // Conversión de datos GPS a payload
    void gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload);
    
    // Conversión de payload a datos GPS
    void payloadToGpsData(const GPSPayload* payload, float* lat, float* lon, uint32_t* timestamp);
    
    /*
     * MÉTODOS PRIVADOS - MESHTASTIC ALGORITHM
     */
    
    // Duplicate Detection
    bool wasSeenRecently(const LoRaPacket* packet);
    void addToRecentPackets(uint16_t sourceID, uint32_t packetID);
    void cleanOldPackets();
    
    // SNR-based Delays
    uint8_t getCWsize(float snr);
    uint32_t getTxDelayMsecWeighted(float snr, DeviceRole role);
    uint32_t getRandomDelay(uint8_t cwSize);
    
    // Mesh Logic
    bool shouldFilterReceived(const LoRaPacket* packet);
    bool isRebroadcaster();
    bool isToUs(const LoRaPacket* packet);
    bool isFromUs(const LoRaPacket* packet);
    bool isBroadcast(uint16_t destinationID);
    
    // Role Priority
    bool hasRolePriority(DeviceRole role);
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    LoRaManager();
    ~LoRaManager();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    
    // Inicialización del sistema LoRa
    bool begin();
    bool begin(uint16_t deviceID);
    
    // Control del módulo
    void update();
    void sleep();
    void wakeup();
    void reset();
    
    // Actualizar frecuencia desde configuración
    bool updateFrequencyFromConfig();
    
    /*
     * TRANSMISIÓN DE DATOS (existentes)
     */
    
    // Enviar datos GPS (para TRACKER)
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp);
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID);
    
    // Enviar packet genérico
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength);
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID);
    
    /*
     * NUEVOS MÉTODOS PARA CONFIGURACIÓN REMOTA
     */
    
    // Discovery de dispositivos
    bool sendDiscoveryRequest();
    bool sendDiscoveryResponse(uint16_t requestorID);
    
    // Comandos remotos
    bool sendRemoteConfigCommand(uint16_t targetID, RemoteCommandType cmdType, uint32_t value, uint32_t sequenceID);
    bool sendRemoteConfigResponse(uint16_t targetID, RemoteCommandType cmdType, bool success, uint32_t sequenceID, uint32_t currentValue, const char* message);
    
    // Procesamiento de comandos remotos
    bool processRemoteConfigCommand(const LoRaPacket* packet);
    bool processRemoteConfigResponse(const LoRaPacket* packet);
    bool processDiscoveryRequest(const LoRaPacket* packet);
    bool processDiscoveryResponse(const LoRaPacket* packet);
    
    /*
     * RECEPCIÓN DE DATOS (existentes)
     */
    
    // Verificar si hay packets disponibles
    bool isPacketAvailable();
    
    // Recibir y procesar packet
    bool receivePacket(LoRaPacket* packet);
    
    // Procesar packet GPS recibido
    bool processGPSPacket(const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID);
    
    /*
     * MESH ROUTING (existentes)
     */
    
    // Enhanced rebroadcast con algoritmo Meshtastic
    bool perhapsRebroadcast(const LoRaPacket* packet);
    
    // Configurar role para mesh priority
    void setRole(DeviceRole role) { currentRole = role; }
    DeviceRole getRole() { return currentRole; }
    
    // Mesh statistics
    uint32_t getDuplicatesIgnored() { return stats.duplicatesIgnored; }
    uint32_t getRebroadcasts() { return stats.rebroadcasts; }
    uint32_t getHopLimitReached() { return stats.hopLimitReached; }
    
    /*
     * CONFIGURACIÓN Y ESTADO (existentes)
     */
    
    // Configurar parámetros de radio
    void setFrequency(float frequency);
    void setTxPower(int8_t power);
    void setBandwidth(float bandwidth);
    void setSpreadingFactor(uint8_t sf);
    
    // Obtener estado y estadísticas
    LoRaStatus getStatus();
    String getStatusString();
    LoRaStats getStats();
    float getLastRSSI();
    float getLastSNR();
    
    // Configurar ID del dispositivo
    void setDeviceID(uint16_t id) { deviceID = id; }
    uint16_t getDeviceID() { return deviceID; }
    
    /*
     * MÉTODOS DE DIAGNÓSTICO (existentes)
     */
    
    // Test de comunicación básica
    bool selfTest();
    
    // Información de debug
    void printConfiguration();
    void printStats();
    void printMeshStats();
    void printPacketInfo(const LoRaPacket* packet);
    
    // Reset de estadísticas
    void resetStats();
};

/*
 * INSTANCIA GLOBAL
 */
extern LoRaManager loraManager;

/*
 * UTILIDADES Y CONSTANTES
 */

// Constantes de timeout
#define LORA_TX_TIMEOUT         5000    // ms - Timeout para transmisión
#define LORA_RX_TIMEOUT         1000    // ms - Timeout para recepción
#define LORA_INIT_TIMEOUT       10000   // ms - Timeout para inicialización

// Direcciones especiales
#define LORA_BROADCAST_ADDR     0xFFFF  // Dirección de broadcast
#define LORA_INVALID_ADDR       0x0000  // Dirección inválida

// Tamaños máximos
#define LORA_MAX_PAYLOAD_SIZE   32      // bytes - Máximo payload por packet
#define LORA_MAX_PACKET_SIZE    64      // bytes - Máximo tamaño total de packet

// Constantes Meshtastic
#define MESHTASTIC_MAX_HOPS     3       // Máximo saltos por defecto
#define MESHTASTIC_PACKET_ID_INVALID 0  // ID inválido

// NUEVAS CONSTANTES para configuración remota
#define REMOTE_CONFIG_TIMEOUT   5000    // ms - Timeout para respuesta remota
#define DISCOVERY_TIMEOUT       3000    // ms - Timeout para discovery

#endif