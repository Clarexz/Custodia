/*
 * NETWORK_SECURITY.H - Sistema de Canales PSK basado en Meshtastic (COMPLETO)
 * 
 * Copiado directamente de: https://github.com/meshtastic/firmware/blob/master/src/mesh/Channels.h
 * Adaptado para nuestro sistema ESP32-S3 + SX1262
 */

#pragma once

#include <Arduino.h>
#include <vector> 
#include <functional>
#include "../config/config_manager.h"

/*
 * DEFINICIONES DE TIPOS (copiadas exactas de Meshtastic)
 */
typedef uint8_t pb_byte_t;
typedef uint32_t pb_size_t;

typedef struct _pb_bytes_array_32 {
    pb_size_t size;
    pb_byte_t bytes[32];
} pb_bytes_array_32_t;

typedef struct _ChannelSettings {
    /**
     * A simple preshared key for now for crypto.
     * Must be exactly 16 or 32 bytes
     */
    pb_bytes_array_32_t psk;

    /**
     * A SHORT name that will be packed into the URL.  Less than 12 bytes.
     * Something for end users to call the channel
     */
    char name[MAX_CHANNEL_NAME_LENGTH];

    /**
     * Used to construct a globally unique channel ID.
     * The full channel ID will be: "name.id" where ID is shown as base36.
     */
    uint32_t id;

    /**
     * If true, messages on the mesh will only be accepted if signed by their sender.
     */
    bool psk_auth;

    /**
     * If true, the channel will be encrypted and the name will be hidden from other nodes
     * on the mesh. This is useful for sensitive channels that should not be discoverable.
     */
    bool encrypted;

    /**
     * If true, the channel will be visible to other nodes on the mesh for discovery purposes.
     * This setting works in conjunction with encrypted to determine channel visibility.
     */
    bool discoverable;

    /**
     * Provides backwards compatibility for channel configuration in older firmware versions.
     * Used to ensure proper migration of settings when upgrading firmware.
     */
    uint32_t legacy_config_version;
} ChannelSettings;

class NetworkSecurity {
public:
    /**
     * Inicializar el sistema de canales
     */
    static void init();

    /**
     * Crear un nuevo canal con PSK generada automáticamente
     * @param name Nombre del canal (máximo 11 caracteres)
     * @return true si se creó exitosamente
     */
    static bool createChannel(const char* name);

    /**
     * Crear un canal con PSK específica
     * @param name Nombre del canal
     * @param psk PSK en formato base64 o hex
     * @return true si se creó exitosamente
     */
    static bool createChannelWithPSK(const char* name, const char* psk);

    /**
     * Unirse a un canal existente (debe tener la PSK guardada)
     * @param name Nombre del canal
     * @return true si se pudo unir
     */
    static bool joinChannel(const char* name);

    /**
     * Unirse a un canal con PSK específica
     * @param name Nombre del canal
     * @param psk PSK en formato base64 o hex
     * @return true si se pudo unir
     */
    static bool joinChannelWithPSK(const char* name, const char* psk);

    /**
     * Eliminar un canal
     * @param name Nombre del canal a eliminar
     * @return true si se eliminó exitosamente
     */
    static bool deleteChannel(const char* name);

    /**
     * Obtener información de un canal
     * @param name Nombre del canal
     * @param settings Estructura donde se guardará la información
     * @return true si se encontró el canal
     */
    static bool getChannelInfo(const char* name, ChannelSettings* settings);

    /**
     * Listar todos los canales configurados
     * @param callback Función que se llama por cada canal encontrado
     */
    static void listChannels(std::function<void(const ChannelSettings&)> callback);

    /**
     * Obtener el nombre del canal activo
     * @return Nombre del canal activo o "default" si no hay ninguno
     */
    static const char* getActiveChannelName();

    /**
     * Obtener el índice del canal activo
     * @return Índice del canal activo o -1 si no hay ninguno
     */
    static int getActiveChannelIndex();

    /**
     * Obtener número total de canales
     * @return Número de canales configurados
     */
    static size_t getChannelCount();

    /**
     * Obtener hash del canal activo (ACTUALIZADO: 32-bit)
     * @return Hash de 32-bit del canal para identificación
     */
    static uint32_t getHash();

    /**
     * Generar hash de un canal específico (ACTUALIZADO: 32-bit)
     * @param ch Configuración del canal
     * @return Hash de 32-bit del canal
     */
    static uint32_t generateHash(const ChannelSettings* ch);

    /**
     * Obtener la clave del canal activo (copiado de Meshtastic)
     * @return Puntero a la clave PSK
     */
    static const uint8_t* getKey();

    /**
     * Obtener el tamaño de la clave del canal activo
     * @return Tamaño de la PSK del canal activo
     */
    static size_t getKeySize();

    /**
     * Obtener configuración del canal activo
     * @return Puntero a la configuración del canal
     */
    static const ChannelSettings* getActiveChannel();

    /**
     * Cambiar al canal especificado
     * @param name Nombre del canal
     * @return true si se cambió exitosamente
     */
    static bool setActiveChannel(const char* name);

    /**
     * Validar si un mensaje pertenece al canal activo (ACTUALIZADO: 32-bit)
     * @param channelHash Hash del canal del mensaje
     * @return true si el mensaje es válido para el canal activo
     */
    static bool isValidForActiveChannel(uint32_t channelHash);

    /**
     * Generar PSK aleatoria usando hardware RNG del ESP32
     * @param psk Buffer donde se guardará la PSK
     * @param length Longitud de la PSK (16 o 32 bytes)
     */
    static void generateRandomPSK(uint8_t* psk, size_t length);

    /**
     * Convertir PSK a string base64 para mostrar
     * @param psk Buffer con la PSK
     * @param length Longitud de la PSK
     * @param output Buffer donde se guardará el string base64
     * @param outputSize Tamaño del buffer de salida
     */
    static void pskToBase64(const uint8_t* psk, size_t length, char* output, size_t outputSize);

    /**
     * Convertir string base64 a PSK
     * @param base64 String en base64
     * @param psk Buffer donde se guardará la PSK
     * @param maxLength Longitud máxima del buffer PSK
     * @return Longitud real de la PSK decodificada
     */
    static size_t base64ToPSK(const char* base64, uint8_t* psk, size_t maxLength);

    /**
     * Función para probar el sistema de hash
     */
    static void testHashGeneration();

    static bool setCryptoForActiveChannel();
    static const uint8_t* getActiveChannelKey();
    static size_t getActiveChannelKeySize();
    static bool isCryptoEnabled();
    static void autoConfigureCrypto();

private:
    static std::vector<ChannelSettings> channels;
    static int activeChannelIndex;
    static bool initialized;

    /**
     * Buscar canal por nombre
     * @param name Nombre del canal
     * @return Índice del canal o -1 si no se encuentra
     */
    static int findChannelByName(const char* name);

    /**
     * Validar configuración de canal
     * @param settings Configuración a validar
     * @return true si es válida
     */
    static bool validateChannelSettings(const ChannelSettings* settings);

    /**
     * Guardar canales en EEPROM (placeholder)
     */
    static void saveChannelsToEEPROM();

    /**
     * Cargar canales desde EEPROM (placeholder)
     */
    static void loadChannelsFromEEPROM();
};