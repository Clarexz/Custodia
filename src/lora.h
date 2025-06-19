/*
 * LORA.H - Sistema LoRa para Custom Meshtastic GPS Tracker
 * 
 * Este archivo define la interfaz del sistema LoRa que maneja la comunicación
 * inalámbrica usando el módulo SX1262 con RadioLib.
 * 
 * Características:
 * - Comunicación punto a punto básica
 * - Protocolo de packets personalizado
 * - Configuración optimizada para asset tracking
 * - Preparado para mesh routing futuro
 */

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <RadioLib.h>

/*
 * CONFIGURACIÓN DE HARDWARE PARA XIAO ESP32S3 + WIO SX1262
 * 
 * Pines oficiales según Meshtastic firmware:
 * Fuente: https://github.com/meshtastic/firmware/blob/v2.5.20.4c97351/variants/seeed_xiao_s3/variant.h
 */

// Configuración oficial de Meshtastic para XIAO S3 + SX1262
#define LORA_SCK_PIN    7   // LORA_SCK
#define LORA_MISO_PIN   8   // LORA_MISO
#define LORA_MOSI_PIN   9   // LORA_MOSI
#define LORA_NSS_PIN    41  // LORA_CS (¡Era 41, no 10!)
#define LORA_DIO1_PIN   39  // LORA_DIO1 (¡Era 39, no 5!)
#define LORA_NRST_PIN   42  // LORA_RESET (¡Era 42, no 6!)
#define LORA_BUSY_PIN   40  // SX126X_BUSY (¡Era 40, no 4!)

/*
 * CONFIGURACIÓN DE RADIO
 */

// Frecuencia para región de México (915 MHz ISM band)
#define LORA_FREQUENCY      915.0f  // MHz

// Configuración optimizada para velocidad vs alcance
#define LORA_BANDWIDTH      125.0f  // kHz - Balance velocidad/alcance
#define LORA_SPREADING_FACTOR   7   // SF7 = Rápido, SF12 = Largo alcance
#define LORA_CODING_RATE        5   // 4/5 = Buena corrección de errores
#define LORA_TX_POWER          14   // dBm - Potencia de transmisión
#define LORA_PREAMBLE_LENGTH    8   // Símbolos de preámbulo
#define LORA_SYNC_WORD     0x12     // Palabra de sincronización personalizada

/*
 * PROTOCOLO DE PACKETS
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
 * ESTADOS Y ESTADÍSTICAS DEL SISTEMA LORA
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
};

/*
 * CLASE PRINCIPAL - LoRaManager
 * 
 * Maneja toda la funcionalidad LoRa incluyendo transmisión,
 * recepción, protocolo de packets y estadísticas.
 */
class LoRaManager {
private:
    // Instancia del módulo SX1262 usando RadioLib
    SX1262 radio;
    
    // Estado actual del sistema LoRa
    LoRaStatus status;
    
    // Estadísticas de comunicación
    LoRaStats stats;
    
    // ID de este dispositivo (obtenido de ConfigManager)
    uint16_t deviceID;
    
    // Contador para generar IDs únicos de packets
    uint32_t packetCounter;
    
    // Buffer para recepción
    uint8_t receiveBuffer[256];
    
    /*
     * MÉTODOS PRIVADOS
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
    void update();                  // Debe llamarse en el loop principal
    void sleep();                   // Modo bajo consumo
    void wakeup();                  // Despertar del sleep
    void reset();                   // Reset del módulo
    
    /*
     * TRANSMISIÓN DE DATOS
     */
    
    // Enviar datos GPS (para TRACKER)
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp);
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID);
    
    // Enviar packet genérico
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength);
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID);
    
    // Retransmitir packet (para REPEATER)
    bool retransmitPacket(const LoRaPacket* packet);
    
    /*
     * RECEPCIÓN DE DATOS
     */
    
    // Verificar si hay packets disponibles
    bool isPacketAvailable();
    
    // Recibir y procesar packet
    bool receivePacket(LoRaPacket* packet);
    
    // Procesar packet GPS recibido
    bool processGPSPacket(const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID);
    
    /*
     * CONFIGURACIÓN Y ESTADO
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
     * MÉTODOS DE DIAGNÓSTICO
     */
    
    // Test de comunicación básica
    bool selfTest();
    
    // Información de debug
    void printConfiguration();
    void printStats();
    void printPacketInfo(const LoRaPacket* packet);
    
    // Reset de estadísticas
    void resetStats();
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en lora.cpp para uso en todo el proyecto
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

#endif