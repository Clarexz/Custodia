/*
 * LORA_COMM.H - Transmisión y Recepción Básica
 * 
 * MODULARIZADO: Declaraciones para comunicación básica LoRa
 * extraídas del lora.h original
 */

#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "lora_types.h"

/*
 * FORWARD DECLARATION
 */
class LoRaManager;

/*
 * MÉTODOS DE TRANSMISIÓN DE DATOS
 */

// Enviar datos GPS (para TRACKER)
bool sendGPSData(LoRaManager* manager, float latitude, float longitude, uint32_t timestamp);
bool sendGPSData(LoRaManager* manager, float latitude, float longitude, uint32_t timestamp, uint16_t destinationID);

// Enviar packet genérico
bool sendPacket(LoRaManager* manager, LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength);
bool sendPacket(LoRaManager* manager, LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID);

/*
 * MÉTODOS DE RECEPCIÓN DE DATOS
 */

// Verificar si hay packets disponibles
bool isPacketAvailable(LoRaManager* manager);

// Recibir y procesar packet
bool receivePacket(LoRaManager* manager, LoRaPacket* packet);

// Procesar packet GPS recibido
bool processGPSPacket(LoRaManager* manager, const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID);

// Loop principal de actualización
void updateComm(LoRaManager* manager);

#endif