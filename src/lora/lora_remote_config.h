/*
 * LORA_REMOTE_CONFIG.H - Configuración Remota y Discovery
 * 
 * MODULARIZADO: Declaraciones para configuración remota y discovery
 * extraídas del lora.h original
 */

#ifndef LORA_REMOTE_CONFIG_H
#define LORA_REMOTE_CONFIG_H

#include "lora_types.h"

/*
 * FORWARD DECLARATION
 */
class LoRaManager;

/*
 * MÉTODOS DE DISCOVERY DE DISPOSITIVOS
 */

// Enviar solicitud de discovery
bool sendDiscoveryRequest(LoRaManager* manager);

// Enviar respuesta de discovery
bool sendDiscoveryResponse(LoRaManager* manager, uint16_t requestorID);

// Procesar solicitud de discovery
bool processDiscoveryRequest(LoRaManager* manager, const LoRaPacket* packet);

// Procesar respuesta de discovery
bool processDiscoveryResponse(LoRaManager* manager, const LoRaPacket* packet);

/*
 * MÉTODOS DE COMANDOS REMOTOS
 */

// Enviar comando de configuración remota
bool sendRemoteConfigCommand(LoRaManager* manager, uint16_t targetID, RemoteCommandType cmdType, uint32_t value, uint32_t sequenceID);

// Enviar respuesta de configuración remota
bool sendRemoteConfigResponse(LoRaManager* manager, uint16_t targetID, RemoteCommandType cmdType, bool success, uint32_t sequenceID, uint32_t currentValue, const char* message);

// Procesar comando de configuración remota
bool processRemoteConfigCommand(LoRaManager* manager, const LoRaPacket* packet);

// Procesar respuesta de configuración remota
bool processRemoteConfigResponse(LoRaManager* manager, const LoRaPacket* packet);

#endif