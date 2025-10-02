/*
 * LORA_MANAGER.H - Clase Principal del Sistema LoRa (CORREGIDA)
 * 
 * CORREGIDO: Solo declaraciones de métodos, sin implementaciones inline
 */

#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h>
#include <vector>
#include "../config/config_manager.h"  // AGREGAR ESTA LÍNEA
#include "lora_types.h"
#include "lora_hardware.h"

/*
 * CLASE PRINCIPAL - LoRaManager
 */
class LoRaManager {
private:
    // === COMPONENTES CORE ===
    SX1262 radio;
    LoRaStatus status;
    LoRaStats stats;
    uint16_t deviceID;
    uint32_t packetCounter;
    uint8_t receiveBuffer[256];
    String lastSimplePacket;
    bool simplePacketPending;
    
    // === COMPONENTES MESHTASTIC ===
    std::vector<PacketRecord> recentBroadcasts;
    DeviceRole currentRole;
    ContentionWindow cw;
    
    /*
     * MÉTODOS PRIVADOS DE HARDWARE
     */
    bool initRadio();
    bool configureRadio();
    
    /*
     * MÉTODOS PRIVADOS DE MESH
     */
    bool wasSeenRecently(const LoRaPacket* packet);
    void addToRecentPackets(uint16_t sourceID, uint32_t packetID);
    void cleanOldPackets();
    uint8_t getCWsize(float snr);
    uint32_t getTxDelayMsecWeighted(float snr, DeviceRole role);
    uint32_t getRandomDelay(uint8_t cwSize);
    bool shouldFilterReceived(const LoRaPacket* packet);
    bool isPacketFromSameNetwork(const LoRaPacket* packet);
    bool isRebroadcaster();
    bool isToUs(const LoRaPacket* packet);
    bool isFromUs(const LoRaPacket* packet);
    bool isBroadcast(uint16_t destinationID);
    bool hasRolePriority(DeviceRole role);
    
    /*
     * MÉTODOS PRIVADOS DE PACKETS
     */
    uint16_t calculateChecksum(const LoRaPacket* packet);
    bool validatePacket(const LoRaPacket* packet);
    void gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload);
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
    bool begin();
    bool begin(uint16_t deviceID);
    void update();
    void sleep();
    void wakeup();
    void reset();
    bool updateFrequencyFromConfig();
    
    /*
     * MÉTODOS DE COMUNICACIÓN
     */
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp);
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID);
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength);
    bool sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID);
    bool isPacketAvailable();
    bool receivePacket(LoRaPacket* packet);
    bool processGPSPacket(const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID);
    bool fetchSimplePacket(String& out);
    
    /*
     * MÉTODOS DE MESH
     */
    bool perhapsRebroadcast(const LoRaPacket* packet);
    void setRole(DeviceRole role);
    DeviceRole getRole();
    uint32_t getDuplicatesIgnored();
    uint32_t getRebroadcasts();
    uint32_t getHopLimitReached();
    
    /*
     * MÉTODOS DE CONFIGURACIÓN REMOTA
     */
    bool sendDiscoveryRequest();
    bool sendDiscoveryResponse(uint16_t requestorID);
    bool sendRemoteConfigCommand(uint16_t targetID, RemoteCommandType cmdType, uint32_t value, uint32_t sequenceID);
    bool sendRemoteConfigResponse(uint16_t targetID, RemoteCommandType cmdType, bool success, uint32_t sequenceID, uint32_t currentValue, const char* message);
    bool processRemoteConfigCommand(const LoRaPacket* packet);
    bool processRemoteConfigResponse(const LoRaPacket* packet);
    bool processDiscoveryRequest(const LoRaPacket* packet);
    bool processDiscoveryResponse(const LoRaPacket* packet);
    
    /*
     * MÉTODOS DE CONFIGURACIÓN
     */
    void setFrequency(float frequency);
    void setTxPower(int8_t power);
    void setBandwidth(float bandwidth);
    void setSpreadingFactor(uint8_t sf);
    void setCodingRate(uint8_t cr);
    void setPreambleLength(uint16_t preamble);
    
    /*
     * GETTERS Y SETTERS
     */
    LoRaStatus getStatus();
    String getStatusString();
    LoRaStats getStats();
    float getLastRSSI();
    float getLastSNR();
    uint16_t getDeviceID();
    void setDeviceID(uint16_t id);
    
    /*
     * DIAGNÓSTICO
     */
    bool selfTest();
    void printConfiguration();
    void printStats();
    void printMeshStats();
    void printPacketInfo(const LoRaPacket* packet);
    void resetStats();
};

/*
 * INSTANCIA GLOBAL
 */
extern LoRaManager loraManager;

#endif
