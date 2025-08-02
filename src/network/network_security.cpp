/*
 * NETWORK_SECURITY.CPP - Implementación del Sistema de Canales PSK
 * 
 * Copiado y adaptado de: https://github.com/meshtastic/firmware/blob/master/src/mesh/Channels.cpp
 * Adaptado para nuestro sistema ESP32-S3
 */

#include "network_security.h"
#include "crypto_engine.h"
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <cstring>

// Variables estáticas
std::vector<ChannelSettings> NetworkSecurity::channels;
int NetworkSecurity::activeChannelIndex = -1;
bool NetworkSecurity::initialized = false;

void NetworkSecurity::init()
{
    if (initialized) return;
    
    Serial.println("[NETWORK] Initializing Network Security...");
    
    // Cargar canales desde EEPROM (placeholder por ahora)
    loadChannelsFromEEPROM();
    
    // Si no hay canales, crear uno por defecto
    if (channels.empty()) {
        Serial.println("[NETWORK] No channels found, creating default channel");
        createChannel("default");
        activeChannelIndex = 0;
    } else {
        // Activar el primer canal encontrado
        activeChannelIndex = 0;
    }
    
    initialized = true;
    Serial.printf("[NETWORK] Network Security initialized with %d channels\n", channels.size());
}

bool NetworkSecurity::createChannel(const char* name)
{
    if (!name || strlen(name) == 0 || strlen(name) >= 12) {
        Serial.println("[NETWORK] Invalid channel name");
        return false;
    }
    
    // Verificar si ya existe
    if (findChannelByName(name) >= 0) {
        Serial.printf("[NETWORK] Channel '%s' already exists\n", name);
        return false;
    }
    
    // Crear nueva configuración
    ChannelSettings newChannel = {};
    strncpy(newChannel.name, name, sizeof(newChannel.name) - 1);
    
    // Generar PSK aleatoria de 32 bytes (AES-256)
    newChannel.psk.size = 32;
    generateRandomPSK(newChannel.psk.bytes, 32);
    
    // Generar ID único basado en el nombre y timestamp
    newChannel.id = 0;
    for (const char* p = name; *p; p++) {
        newChannel.id = (newChannel.id * 31) + *p;
    }
    newChannel.id ^= (uint32_t)millis(); // XOR con timestamp para unicidad
    
    // Configuraciones por defecto
    newChannel.psk_auth = false;
    newChannel.encrypted = true;
    newChannel.discoverable = false;
    newChannel.legacy_config_version = 1;
    
    // Validar y agregar
    if (!validateChannelSettings(&newChannel)) {
        Serial.println("[NETWORK] Generated channel settings are invalid");
        return false;
    }
    
    channels.push_back(newChannel);
    saveChannelsToEEPROM();
    
    Serial.printf("[NETWORK] Created channel '%s' with ID %u\n", name, newChannel.id);
    return true;
}

bool NetworkSecurity::createChannelWithPSK(const char* name, const char* psk)
{
    if (!name || strlen(name) == 0 || strlen(name) >= 12) {
        Serial.println("[NETWORK] Invalid channel name");
        return false;
    }
    
    if (!psk || strlen(psk) == 0) {
        Serial.println("[NETWORK] Invalid PSK");
        return false;
    }
    
    // Verificar si ya existe
    if (findChannelByName(name) >= 0) {
        Serial.printf("[NETWORK] Channel '%s' already exists\n", name);
        return false;
    }
    
    // Crear nueva configuración
    ChannelSettings newChannel = {};
    strncpy(newChannel.name, name, sizeof(newChannel.name) - 1);
    
    // Decodificar PSK desde base64
    uint8_t decodedPSK[32];
    size_t pskLength = base64ToPSK(psk, decodedPSK, sizeof(decodedPSK));
    
    if (pskLength != 16 && pskLength != 32) {
        Serial.printf("[NETWORK] PSK must be 16 or 32 bytes (got %d)\n", pskLength);
        return false;
    }
    
    newChannel.psk.size = pskLength;
    memcpy(newChannel.psk.bytes, decodedPSK, pskLength);
    
    // Generar ID basado en nombre
    newChannel.id = 0;
    for (const char* p = name; *p; p++) {
        newChannel.id = (newChannel.id * 31) + *p;
    }
    
    // Configuraciones por defecto
    newChannel.psk_auth = false;
    newChannel.encrypted = true;
    newChannel.discoverable = false;
    newChannel.legacy_config_version = 1;
    
    // Validar y agregar
    if (!validateChannelSettings(&newChannel)) {
        Serial.println("[NETWORK] Channel settings are invalid");
        return false;
    }
    
    channels.push_back(newChannel);
    saveChannelsToEEPROM();
    
    Serial.printf("[NETWORK] Created channel '%s' with custom PSK\n", name);
    return true;
}

bool NetworkSecurity::joinChannel(const char* name)
{
    int index = findChannelByName(name);
    if (index < 0) {
        Serial.printf("[NETWORK] Channel '%s' not found\n", name);
        return false;
    }
    
    activeChannelIndex = index;
    Serial.printf("[NETWORK] Joined channel '%s'\n", name);
    return true;
}

bool NetworkSecurity::joinChannelWithPSK(const char* name, const char* psk)
{
    // Si el canal no existe, crearlo
    if (findChannelByName(name) < 0) {
        if (!createChannelWithPSK(name, psk)) {
            return false;
        }
    }
    
    return joinChannel(name);
}

bool NetworkSecurity::deleteChannel(const char* name)
{
    int index = findChannelByName(name);
    if (index < 0) {
        Serial.printf("[NETWORK] Channel '%s' not found\n", name);
        return false;
    }
    
    // No permitir eliminar el canal activo si es el único
    if (channels.size() == 1) {
        Serial.println("[NETWORK] Cannot delete the only channel");
        return false;
    }
    
    // Si estamos eliminando el canal activo, cambiar a otro
    if (index == activeChannelIndex) {
        activeChannelIndex = (index == 0) ? 1 : 0;
    } else if (index < activeChannelIndex) {
        activeChannelIndex--;
    }
    
    channels.erase(channels.begin() + index);
    saveChannelsToEEPROM();
    
    Serial.printf("[NETWORK] Deleted channel '%s'\n", name);
    return true;
}

bool NetworkSecurity::getChannelInfo(const char* name, ChannelSettings* settings)
{
    int index = findChannelByName(name);
    if (index < 0) {
        return false;
    }
    
    if (settings) {
        *settings = channels[index];
    }
    
    return true;
}

void NetworkSecurity::listChannels(std::function<void(const ChannelSettings&)> callback)
{
    for (const auto& channel : channels) {
        callback(channel);
    }
}

// Funciones copiadas directamente de Meshtastic Channels.cpp
uint8_t NetworkSecurity::getHash()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return 0;
    }
    
    return generateHash(&channels[activeChannelIndex]);
}

uint8_t NetworkSecurity::generateHash(const ChannelSettings* ch)
{
    if (!ch || ch->psk.size == 0) {
        return 0;
    }
    
    // Algoritmo copiado exacto de Meshtastic
    uint8_t hash = 0;
    
    // XOR all the PSK bytes
    for (size_t i = 0; i < ch->psk.size; i++) {
        hash ^= ch->psk.bytes[i];
    }
    
    // XOR with the channel name
    for (size_t i = 0; i < strlen(ch->name); i++) {
        hash ^= ch->name[i];
    }
    
    // Ensure we never return 0 (reserved for no encryption)
    if (hash == 0) {
        hash = 1;
    }
    
    return hash;
}

const uint8_t* NetworkSecurity::getKey()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return nullptr;
    }
    
    return channels[activeChannelIndex].psk.bytes;
}

const ChannelSettings* NetworkSecurity::getActiveChannel()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return nullptr;
    }
    
    return &channels[activeChannelIndex];
}

bool NetworkSecurity::setActiveChannel(const char* name)
{
    return joinChannel(name);
}

bool NetworkSecurity::isValidForActiveChannel(uint8_t channelHash)
{
    return channelHash == getHash();
}

void NetworkSecurity::generateRandomPSK(uint8_t* psk, size_t length)
{
    if (!psk || (length != 16 && length != 32)) {
        Serial.println("[NETWORK] Invalid PSK parameters");
        return;
    }
    
    // Usar hardware RNG del ESP32 para máxima entropía
    esp_fill_random(psk, length);
    
    Serial.printf("[NETWORK] Generated %d-byte PSK using ESP32 hardware RNG\n", length);
}

void NetworkSecurity::pskToBase64(const uint8_t* psk, size_t length, char* output, size_t outputSize)
{
    if (!psk || !output || outputSize < ((length * 4 / 3) + 4)) {
        Serial.println("[NETWORK] Invalid base64 conversion parameters");
        return;
    }
    
    size_t olen;
    int ret = mbedtls_base64_encode((unsigned char*)output, outputSize, &olen, psk, length);
    
    if (ret != 0) {
        Serial.printf("[NETWORK] Base64 encoding failed: %d\n", ret);
        output[0] = '\0';
    }
}

size_t NetworkSecurity::base64ToPSK(const char* base64, uint8_t* psk, size_t maxLength)
{
    if (!base64 || !psk) {
        Serial.println("[NETWORK] Invalid base64 decode parameters");
        return 0;
    }
    
    size_t olen;
    int ret = mbedtls_base64_decode(psk, maxLength, &olen, 
                                   (const unsigned char*)base64, strlen(base64));
    
    if (ret != 0) {
        Serial.printf("[NETWORK] Base64 decoding failed: %d\n", ret);
        return 0;
    }
    
    return olen;
}

// Funciones privadas
int NetworkSecurity::findChannelByName(const char* name)
{
    if (!name) return -1;
    
    for (size_t i = 0; i < channels.size(); i++) {
        if (strcmp(channels[i].name, name) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

bool NetworkSecurity::validateChannelSettings(const ChannelSettings* settings)
{
    if (!settings) return false;
    
    // Validar nombre
    if (strlen(settings->name) == 0 || strlen(settings->name) >= 12) {
        Serial.println("[NETWORK] Channel name must be 1-11 characters");
        return false;
    }
    
    // Validar PSK
    if (settings->psk.size != 16 && settings->psk.size != 32) {
        Serial.printf("[NETWORK] PSK must be 16 or 32 bytes (got %d)\n", settings->psk.size);
        return false;
    }
    
    return true;
}

void NetworkSecurity::saveChannelsToEEPROM()
{
    // Por ahora, placeholder - en el futuro se integrará con config_manager
    Serial.printf("[NETWORK] Saving %d channels to EEPROM (placeholder)\n", channels.size());
}

void NetworkSecurity::loadChannelsFromEEPROM()
{
    // Por ahora, placeholder - en el futuro se integrará con config_manager
    Serial.println("[NETWORK] Loading channels from EEPROM (placeholder)");
}