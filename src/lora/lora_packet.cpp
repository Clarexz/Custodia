/*
 * LORA_PACKET.CPP - Procesamiento y Validación de Packets
 * 
 * Este archivo contiene las funciones de utilidad para
 * crear, validar y procesar packets LoRa.
 */

#include "../lora.h"

/*
 * CÁLCULO DE CHECKSUM
 */
uint16_t LoRaManager::calculateChecksum(const LoRaPacket* packet) {
    // Checksum simple XOR de todos los bytes excepto el campo checksum
    uint16_t checksum = 0;
    uint8_t* data = (uint8_t*)packet;
    
    // Calcular XOR de todos los bytes excepto los últimos 2 (checksum)
    for (int i = 0; i < sizeof(LoRaPacket) - 2; i++) {
        checksum ^= data[i];
    }
    
    return checksum;
}

/*
 * VALIDACIÓN DE PACKET
 */
bool LoRaManager::validatePacket(const LoRaPacket* packet) {
    // Verificar checksum
    uint16_t calculatedChecksum = calculateChecksum(packet);
    return (calculatedChecksum == packet->checksum);
}

/*
 * CONVERSIÓN DE DATOS GPS A PAYLOAD
 */
void LoRaManager::gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload) {
    payload->latitude = lat;
    payload->longitude = lon;
    payload->timestamp = timestamp;
    payload->batteryVoltage = 3300;  // 3.3V por defecto (futuro: leer real)
    payload->satellites = 8;         // Simulado
    payload->reserved = 0;
}

/*
 * CONVERSIÓN DE PAYLOAD A DATOS GPS
 */
void LoRaManager::payloadToGpsData(const GPSPayload* payload, float* lat, float* lon, uint32_t* timestamp) {
    *lat = payload->latitude;
    *lon = payload->longitude;
    *timestamp = payload->timestamp;
}

/*
 * IMPRIMIR INFORMACIÓN DEL PACKET
 */
void LoRaManager::printPacketInfo(const LoRaPacket* packet) {
    Serial.println("\n[LoRa] === INFO DEL PACKET ===");
    Serial.println("Tipo: " + String(packet->messageType));
    Serial.println("Origen: " + String(packet->sourceID));
    Serial.println("Destino: " + String(packet->destinationID));
    Serial.println("Saltos: " + String(packet->hops) + "/" + String(packet->maxHops));
    Serial.println("Packet ID: " + String(packet->packetID));
    Serial.println("Payload: " + String(packet->payloadLength) + " bytes");
    Serial.println("Checksum: 0x" + String(packet->checksum, HEX));
    Serial.println("=========================");
}