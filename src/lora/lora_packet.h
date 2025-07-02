/*
 * LORA_PACKET.H - Procesamiento y Validación de Packets
 * 
 * MODULARIZADO: Declaraciones para procesamiento de packets
 * extraídas del lora.h original
 */

#ifndef LORA_PACKET_H
#define LORA_PACKET_H

#include "lora_types.h"

/*
 * FORWARD DECLARATION
 */
class LoRaManager;

/*
 * MÉTODOS DE PROCESAMIENTO DE PACKETS
 */

// Cálculo de checksum para validación
uint16_t calculateChecksum(const LoRaPacket* packet);

// Validación de packet recibido
bool validatePacket(const LoRaPacket* packet);

// Conversión de datos GPS a payload
void gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload);

// Conversión de payload a datos GPS
void payloadToGpsData(const GPSPayload* payload, float* lat, float* lon, uint32_t* timestamp);

// Imprimir información del packet para debug
void printPacketInfo(const LoRaPacket* packet);

#endif