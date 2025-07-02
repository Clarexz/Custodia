/*
 * LORA_MESH.H - Algoritmo Meshtastic Completo
 * 
 * MODULARIZADO: Declaraciones para el algoritmo de Managed Flood Routing
 * extraídas del lora.h original
 */

#ifndef LORA_MESH_H
#define LORA_MESH_H

#include "lora_types.h"

/*
 * FORWARD DECLARATIONS
 */
class LoRaManager;
enum DeviceRole;

/*
 * MÉTODOS DE DUPLICATE DETECTION
 */

// Verificar si un packet fue visto recientemente
bool wasSeenRecently(LoRaManager* manager, const LoRaPacket* packet);

// Agregar packet a la memoria de packets recientes
void addToRecentPackets(LoRaManager* manager, uint16_t sourceID, uint32_t packetID);

// Limpiar packets antiguos de la memoria
void cleanOldPackets(LoRaManager* manager);

/*
 * MÉTODOS DE SNR-BASED DELAYS
 */

// Obtener tamaño de contention window basado en SNR
uint8_t getCWsize(float snr);

// Calcular delay ponderado por SNR y role
uint32_t getTxDelayMsecWeighted(float snr, DeviceRole role);

// Obtener delay aleatorio
uint32_t getRandomDelay(uint8_t cwSize);

/*
 * MÉTODOS DE MESH LOGIC
 */

// Verificar si un packet debe ser filtrado
bool shouldFilterReceived(LoRaManager* manager, const LoRaPacket* packet);

// Verificar si el dispositivo puede retransmitir
bool isRebroadcaster(LoRaManager* manager);

// Verificar si el packet es para nosotros
bool isToUs(LoRaManager* manager, const LoRaPacket* packet);

// Verificar si el packet es de nosotros
bool isFromUs(LoRaManager* manager, const LoRaPacket* packet);

// Verificar si es dirección de broadcast
bool isBroadcast(uint16_t destinationID);

// Verificar si el role tiene prioridad
bool hasRolePriority(DeviceRole role);

/*
 * MÉTODO PRINCIPAL DE REBROADCAST
 */

// Enhanced rebroadcast con algoritmo Meshtastic completo
bool perhapsRebroadcast(LoRaManager* manager, const LoRaPacket* packet);

/*
 * CONFIGURACIÓN DE ROLE PARA MESH
 */

// Configurar role para mesh priority
void setRole(LoRaManager* manager, DeviceRole role);
DeviceRole getRole(LoRaManager* manager);

/*
 * ESTADÍSTICAS MESH
 */

// Obtener estadísticas de mesh
uint32_t getDuplicatesIgnored(LoRaManager* manager);
uint32_t getRebroadcasts(LoRaManager* manager);
uint32_t getHopLimitReached(LoRaManager* manager);

#endif