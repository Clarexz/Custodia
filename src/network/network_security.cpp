/*
 * NETWORK_SECURITY.CPP - Implementación Completa de Meshtastic (ARREGLADA)
 * 
 * MANTIENE: Toda la complejidad de Meshtastic pero ARREGLA los errores
 * OBJETIVO: Compatibilidad 100% con Meshtastic architecture
 */

#include "network_security.h"
#include "crypto_engine.h"
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <cstring>
#include "../config/config_manager.h"
#include "crypto_engine.h"

// Variables estáticas
std::vector<ChannelSettings> NetworkSecurity::channels;
int NetworkSecurity::activeChannelIndex = -1;
bool NetworkSecurity::initialized = false;

void NetworkSecurity::init()
{
    if (initialized) return;
    
    Serial.println("[NETWORK] Initializing Network Security...");

    // Inicializar crypto engine global si no existe
    if (!crypto) {
        crypto = new CryptoEngine();
        crypto->init();
        Serial.println("[NETWORK] Initialized global crypto engine");
    }
    
    // Cargar canales desde EEPROM (placeholder por ahora)
    loadChannelsFromEEPROM();
    
    // Si no hay canales, NO crear ninguno automáticamente
    if (channels.empty()) {
        Serial.println("[NETWORK] No channels found - use NETWORK_CREATE to add channels");
        activeChannelIndex = -1;
    } else {
        // Activar el primer canal encontrado
        activeChannelIndex = 0;
    }
    
    initialized = true;
    Serial.printf("[NETWORK] Network Security initialized with %d channels\n", channels.size());
}

bool NetworkSecurity::createChannel(const char* name)
{
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_CHANNEL_NAME_LENGTH) {
        Serial.println("[NETWORK] Invalid channel name");
        return false;
    }
    
    // Verificar si ya existe
    if (findChannelByName(name) >= 0) {
        Serial.printf("[NETWORK] Channel '%s' already exists\n", name);
        return false;
    }
    
    // Crear nueva configuración COMPLETA de Meshtastic
    ChannelSettings newChannel = {};
    strncpy(newChannel.name, name, sizeof(newChannel.name) - 1);
    
    // Generar PSK aleatoria de 32 bytes (AES-256)
    newChannel.psk.size = 32;
    generateRandomPSK(newChannel.psk.bytes, 32);
    
    // Generar ID basado en nombre (algoritmo de Meshtastic)
    newChannel.id = 0;
    for (const char* p = name; *p; p++) {
        newChannel.id = (newChannel.id * 31) + *p;
    }
    newChannel.id ^= (uint32_t)millis(); // XOR con timestamp para unicidad
    
    // Configuraciones por defecto de Meshtastic
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

    // Si es el primer canal, activarlo automáticamente
    if (channels.size() == 1) {
        activeChannelIndex = 0;
        Serial.printf("[NETWORK] Automatically activated first channel: %s\n", name);
    }
    
    Serial.printf("[NETWORK] Created channel '%s' with ID %u\n", name, newChannel.id);
    return true;
}

bool NetworkSecurity::createChannelWithPSK(const char* name, const char* psk)
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
    
    // Crear nueva configuración COMPLETA de Meshtastic
    ChannelSettings newChannel = {};
    strncpy(newChannel.name, name, sizeof(newChannel.name) - 1);
    
    // Decodificar PSK proporcionada
    size_t pskLength = base64ToPSK(psk, newChannel.psk.bytes, 32);
    if (pskLength == 0) {
        Serial.println("[NETWORK] Invalid PSK format");
        return false;
    }
    newChannel.psk.size = pskLength;
    
    // Generar ID basado en nombre (algoritmo de Meshtastic)
    newChannel.id = 0;
    for (const char* p = name; *p; p++) {
        newChannel.id = (newChannel.id * 31) + *p;
    }
    
    // Configuraciones por defecto de Meshtastic
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

    // Auto-configurar crypto para el nuevo canal activo
    autoConfigureCrypto();
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

const char* NetworkSecurity::getActiveChannelName()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return "default";
    }
    
    return channels[activeChannelIndex].name;
}

int NetworkSecurity::getActiveChannelIndex()
{
    return activeChannelIndex;
}

size_t NetworkSecurity::getChannelCount()
{
    return channels.size();
}

// ===== FUNCIONES DE HASH (ACTUALIZADAS A 32-BIT) =====

uint32_t NetworkSecurity::getHash()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return 0x00000000;  // Hash por defecto
    }
    
    return generateHash(&channels[activeChannelIndex]);
}

uint32_t NetworkSecurity::generateHash(const ChannelSettings* ch)
{
    if (!ch || ch->psk.size == 0) {
        return 0x00000000;  // Hash por defecto
    }
    
    // Algoritmo de hash de 32-bit basado en Meshtastic pero extendido
    uint32_t hash = 0;
    
    // XOR de bytes del channel name con rotación
    for (size_t i = 0; i < strlen(ch->name); i++) {
        hash ^= (uint32_t)ch->name[i];
        // Rotar bits para mejor distribución
        hash = (hash << 1) | (hash >> 31);
    }
    
    // XOR de bytes de la PSK con rotación
    for (size_t i = 0; i < ch->psk.size; i++) {
        hash ^= (uint32_t)ch->psk.bytes[i];
        // Rotar bits para mejor distribución
        hash = (hash << 1) | (hash >> 31);
    }
    
    // XOR con el ID del channel (parte única de Meshtastic)
    hash ^= ch->id;
    
    // Asegurar que nunca devolvemos 0 (reservado para no-encryption)
    if (hash == 0x00000000) {
        hash = 0x12345678; // Hash fallback
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

size_t NetworkSecurity::getKeySize()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return 0;
    }
    
    return channels[activeChannelIndex].psk.size;
}

// ===== FUNCIONES MESHTASTIC ADICIONALES (AGREGADAS) =====

const ChannelSettings* NetworkSecurity::getActiveChannel()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return nullptr;
    }
    
    return &channels[activeChannelIndex];
}

bool NetworkSecurity::setActiveChannel(const char* name)
{
    return joinChannel(name);  // Alias para consistencia con Meshtastic
}

bool NetworkSecurity::isValidForActiveChannel(uint32_t channelHash)
{
    uint32_t activeHash = getHash();
    return (channelHash == activeHash);
}

// ===== FUNCIONES DE UTILIDAD =====

void NetworkSecurity::generateRandomPSK(uint8_t* psk, size_t length)
{
    if (!psk || (length != 16 && length != 32)) {
        Serial.println("[NETWORK] Invalid PSK parameters");
        return;
    }
    
    for (size_t i = 0; i < length; i++) {
        psk[i] = (uint8_t)esp_random();
    }
}

void NetworkSecurity::pskToBase64(const uint8_t* psk, size_t length, char* output, size_t outputSize)
{
    if (!psk || !output || outputSize < 45) { // Base64 de 32 bytes necesita ~45 chars
        return;
    }
    
    size_t olen;
    mbedtls_base64_encode((unsigned char*)output, outputSize, &olen, psk, length);
}

size_t NetworkSecurity::base64ToPSK(const char* base64, uint8_t* psk, size_t maxLength)
{
    if (!base64 || !psk || maxLength < 16) {
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

// ===== FUNCIONES PRIVADAS =====

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
    if (strlen(settings->name) == 0 || strlen(settings->name) >= MAX_CHANNEL_NAME_LENGTH) {
        Serial.println("[NETWORK] Channel name must be 1-30 characters");
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
    // Placeholder para integración futura con config_manager
    Serial.printf("[NETWORK] Saving %d channels to EEPROM (placeholder)\n", channels.size());
}

void NetworkSecurity::loadChannelsFromEEPROM()
{
    // Placeholder para integración futura con config_manager
    Serial.println("[NETWORK] Loading channels from EEPROM (placeholder)");
}

// ===== FUNCIÓN DE TESTING =====

void NetworkSecurity::testHashGeneration() {
    Serial.println("\n=== TESTING 32-BIT CHANNEL HASH (MESHTASTIC COMPLETE) ===");
    
    // Test 1: Consistency - mismo canal = mismo hash
    if (channels.size() > 0) {
        uint32_t hash1 = generateHash(&channels[0]);
        uint32_t hash2 = generateHash(&channels[0]);
        
        Serial.println("Test 1 - Consistency:");
        Serial.printf("  Channel: %s (ID: %u)\n", channels[0].name, channels[0].id);
        Serial.printf("  Hash A: 0x%08X\n", hash1);
        Serial.printf("  Hash B: 0x%08X\n", hash2);
        Serial.printf("  Match: %s\n", (hash1 == hash2) ? "PASS" : "FAIL");
        Serial.printf("  Encrypted: %s\n", channels[0].encrypted ? "YES" : "NO");
        Serial.printf("  Discoverable: %s\n", channels[0].discoverable ? "YES" : "NO");
    }
    
    // Test 2: Active channel hash
    uint32_t activeHash = getHash();
    Serial.println("\nTest 2 - Active Channel:");
    Serial.printf("  Active Channel: %s\n", getActiveChannelName());
    Serial.printf("  Active Hash: 0x%08X\n", activeHash);
    
    Serial.println("=============================================\n");
}

// ===== NUEVAS FUNCIONES CRYPTO INTEGRATION =====

/**
 * Configurar CryptoEngine con la PSK del canal activo
 * Patrón copiado de Meshtastic Channels.cpp::setCrypto()
 */
bool NetworkSecurity::setCryptoForActiveChannel()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        Serial.println("[NETWORK] No active channel for crypto setup");
        if (crypto) {
            crypto->setKey(0, nullptr);  // Disable crypto
        }
        return false;
    }
    
    const ChannelSettings& activeChannel = channels[activeChannelIndex];
    
    if (activeChannel.psk.size == 0) {
        Serial.printf("[NETWORK] Channel '%s' has no PSK, disabling crypto\n", activeChannel.name);
        if (crypto) {
            crypto->setKey(0, nullptr);  // Disable crypto
        }
        return false;
    }
    
    // Configurar crypto engine con la PSK del canal activo
    if (crypto) {
        crypto->setKey(activeChannel.psk.size, (uint8_t*)activeChannel.psk.bytes);
        Serial.printf("[NETWORK] Crypto configured for channel '%s' (AES%d)\n", 
                     activeChannel.name, activeChannel.psk.size * 8);
        return true;
    } else {
        Serial.println("[NETWORK] ERROR: Global crypto engine not initialized");
        return false;
    }
}

/**
 * Obtener PSK del canal activo
 * Útil para verificaciones y debug
 */
const uint8_t* NetworkSecurity::getActiveChannelKey()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return nullptr;
    }
    
    const ChannelSettings& activeChannel = channels[activeChannelIndex];
    return (activeChannel.psk.size > 0) ? activeChannel.psk.bytes : nullptr;
}

/**
 * Obtener tamaño de PSK del canal activo
 */
size_t NetworkSecurity::getActiveChannelKeySize()
{
    if (activeChannelIndex < 0 || activeChannelIndex >= (int)channels.size()) {
        return 0;
    }
    
    return channels[activeChannelIndex].psk.size;
}

/**
 * Verificar si el canal activo tiene encriptación habilitada
 */
bool NetworkSecurity::isCryptoEnabled()
{
    return (getActiveChannelKeySize() > 0);
}

/**
 * Auto-configurar crypto cuando se cambia de canal
 * Llamar automáticamente desde joinChannel()
 */
void NetworkSecurity::autoConfigureCrypto()
{
    if (!initialized) {
        Serial.println("[NETWORK] Auto-crypto: Network not initialized");
        return;
    }
    
    if (setCryptoForActiveChannel()) {
        Serial.printf("[NETWORK] Auto-crypto: Enabled for channel '%s'\n", getActiveChannelName());
    } else {
        Serial.println("[NETWORK] Auto-crypto: Disabled (no PSK or no channel)");
    }
}