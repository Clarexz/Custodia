/*
 * CONFIG_MANAGER.CPP - Core del Sistema de Configuración
 * 
 * MODULARIZADO: Solo funciones core (constructor, begin, load/save, getters/setters)
 * Los comandos están en config_commands.cpp
 */

#include "config_manager.h"
#include "config_commands.h"

/*
 * DEFINICIONES Y CONSTANTES
 */

// Versión del firmware - se actualiza con cada release
#define FIRMWARE_VERSION "0.4.0"

// Tiempo de espera al inicio para entrar en modo configuración
#define STARTUP_CONFIG_WAIT 5000

/*
 * INSTANCIA GLOBAL
 */
ConfigManager configManager;

/*
 * CONSTRUCTOR
 */
ConfigManager::ConfigManager() {
    currentState = STATE_BOOT;

    channelCount = 0;
    activeChannelIndex = -1;
    
    // Limpiar array de channels
    for (int i = 0; i < MAX_CHANNELS; i++) {
        networkChannels[i].name = "";
        networkChannels[i].psk = "";
        networkChannels[i].active = false;
    }
}

/*
 * MÉTODO PRINCIPAL DE INICIALIZACIÓN
 */
void ConfigManager::begin() {
    // Inicializar sistema de preferencias con namespace "mesh-config"
    if (!preferences.begin("mesh-config", false)) {
        Serial.println("[ERROR] No se pudo inicializar sistema de preferencias");
        return;
    }
    
    // Cargar configuración existente desde EEPROM
    loadConfig();
    
    // Mostrar mensaje de bienvenida con información del sistema
    printWelcome();
    
    // Lógica de arranque: determinar estado inicial
    if (!config.configValid) {
        currentState = STATE_CONFIG_MODE;
        Serial.println("[INFO] Dispositivo sin configurar. Entrando en modo configuración.");
        Serial.println("[INFO] Use el comando 'HELP' para ver comandos disponibles.");
        printPrompt();
    } else {
        Serial.println("[INFO] Configuración válida encontrada.");
        printConfig();
        
        // NUEVO: Aplicar perfil LoRa cargado
        if (config.radioProfile >= PROFILE_DESERT_LONG_FAST && config.radioProfile <= PROFILE_CUSTOM_ADVANCED) {
            radioProfileManager.applyProfile(config.radioProfile);
            Serial.println("[INFO] Perfil LoRa aplicado: " + getRadioProfileName());
        }
        
        Serial.println("[INFO] Iniciando en modo operativo en 5 segundos...");
        Serial.println("[INFO] Envie cualquier comando para entrar en modo configuración.");
        
        // Esperar 5 segundos para comandos de configuración
        unsigned long startTime = millis();
        while (millis() - startTime < STARTUP_CONFIG_WAIT) {
            if (Serial.available()) {
                currentState = STATE_CONFIG_MODE;
                Serial.println("\n[INFO] Entrando en modo configuración.");
                printPrompt();
                return;
            }
            delay(100);
        }
        
        // No se recibieron comandos - iniciar modo operativo
        currentState = STATE_RUNNING;
        //Serial.println("[INFO] Iniciando modo operativo...");
    }
}

/*
 * PROCESADOR PRINCIPAL DE COMANDOS SERIALES
 */
void ConfigManager::processSerialInput() {
    if (!Serial.available()) {
        return;
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    
    if (input.length() == 0) {
        printPrompt();
        return;
    }
    
    // Echo del comando para confirmación visual
    Serial.println(">" + input);
    
    /*
     * PARSER DE COMANDOS - Delegar a config_commands.cpp
     */
    
    if (input.startsWith("CONFIG_ROLE ")) {
        handleConfigRole(input.substring(12));
    }
    else if (input.startsWith("CONFIG_DEVICE_ID ")) {
        handleConfigDeviceID(input.substring(17));
    }
    else if (input.startsWith("CONFIG_GPS_INTERVAL ")) {
        handleConfigGpsInterval(input.substring(20));
    }
    else if (input.startsWith("CONFIG_MAX_HOPS ")) {
        handleConfigMaxHops(input.substring(16));
    }
    else if (input.startsWith("CONFIG_DATA_MODE ")) {
        handleConfigDataMode(input.substring(17));
    }
    else if (input.startsWith("CONFIG_REGION ")) {
        handleConfigRegion(input.substring(14));
    }
    
    // ========== COMANDOS DE RADIO PROFILES ==========
    else if (input.startsWith("CONFIG_RADIO_PROFILE ")) {
        handleConfigRadioProfile(input.substring(21));
    }
    else if (input.startsWith("RADIO_PROFILE_CUSTOM ")) {
        String params = input.substring(21);
        int spaceIndex = params.indexOf(' ');
        if (spaceIndex > 0) {
            String param = params.substring(0, spaceIndex);
            String value = params.substring(spaceIndex + 1);
            handleRadioProfileCustom(param, value);
        } else {
            Serial.println("[ERROR] Formato: RADIO_PROFILE_CUSTOM <param> <value>");
            Serial.println("[INFO] Parámetros: SF, BW, CR, POWER, PREAMBLE");
        }
    }
    else if (input == "RADIO_PROFILE_APPLY") {
        handleRadioProfileApply();
    }
    else if (input == "RADIO_PROFILE_STATUS") {
        handleRadioProfileStatus();
    }
    
    // ========== BLOQUE D: COMANDOS NETWORK_* ==========
    else if (input.startsWith("NETWORK_CREATE ")) {
        handleNetworkCreate(input.substring(15));
    }
    else if (input.startsWith("NETWORK_JOIN ")) {
        handleNetworkJoin(input.substring(13));
    }
    else if (input == "NETWORK_LIST") {
        handleNetworkList();
    }
    else if (input.startsWith("NETWORK_INFO ")) {
        handleNetworkInfo(input.substring(13));
    }
    else if (input.startsWith("NETWORK_DELETE ")) {
        handleNetworkDelete(input.substring(15));
    }
    // ==================================================
    
    else if (input.startsWith("MODE ")) {
        handleModeChange(input.substring(5));
    }
    else if (input == "CONFIG_SAVE") {
        handleConfigSave();
    }
    else if (input == "CONFIG_RESET") {
        handleConfigReset();
    }
    else if (input == "INFO") {
        handleInfo();
    }
    else if (input == "STATUS") {
        handleStatus();
    }
    else if (input == "HELP") {
        handleHelp();
    }
    else if (input.startsWith("Q_CONFIG ")) {
        handleQuickConfig(input.substring(9));
    }
    else if (input == "START") {
        if (config.configValid) {
            currentState = STATE_RUNNING;
            Serial.println("[OK] Iniciando modo operativo...");
            Serial.println("[INFO] Comandos disponibles durante operación: MODE SIMPLE, MODE ADMIN");
            return;
        } else {
            Serial.println("[ERROR] Configuración inválida. Configure el dispositivo primero.");
        }
    }
    else {
        Serial.println("[ERROR] Comando desconocido. Use 'HELP' para ver comandos disponibles.");
    }
    
    printPrompt();
}

/*
 * CARGA DE CONFIGURACIÓN DESDE EEPROM
 */
void ConfigManager::loadConfig() {
    config.role = (DeviceRole)preferences.getUChar("role", ROLE_NONE);
    config.deviceID = preferences.getUShort("deviceID", 0);
    config.gpsInterval = preferences.getUShort("gpsInterval", 30);
    config.maxHops = preferences.getUChar("maxHops", 3);
    config.dataMode = (DataDisplayMode)preferences.getUChar("dataMode", DATA_MODE_ADMIN);
    config.region = (LoRaRegion)preferences.getUChar("region", REGION_US);
    config.configValid = preferences.getBool("configValid", false);
    config.radioProfile = (RadioProfile)preferences.getUChar("radioProfile", PROFILE_MESH_MAX_NODES);
    
    strncpy(config.version, FIRMWARE_VERSION, sizeof(config.version) - 1);
    config.version[sizeof(config.version) - 1] = '\0';
    
    if (config.role == ROLE_NONE || config.deviceID == 0) {
        config.configValid = false;
    }

    // ============== NUEVO: CARGAR NETWORK CHANNELS DESDE EEPROM ==============
    // IMPORTANTE: Basado en pattern de Meshtastic NodeDB.cpp
    
    // Cargar cantidad de channels
    channelCount = preferences.getUChar(EEPROM_CHANNEL_COUNT_KEY, 0);
    
    // Cargar índice del channel activo
    activeChannelIndex = preferences.getChar(EEPROM_ACTIVE_CHANNEL_KEY, -1);
    
    // Cargar cada channel individual
    for (int i = 0; i < channelCount && i < MAX_CHANNELS; i++) {
        // Construir keys dinámicamente: "ch_0_name", "ch_1_name", etc.
        String nameKey = String(EEPROM_CHANNEL_NAME_PREFIX) + String(i) + "_name";
        String pskKey = String(EEPROM_CHANNEL_PSK_PREFIX) + String(i);
        
        // Cargar nombre del channel
        String channelName = preferences.getString(nameKey.c_str(), "");
        if (channelName.length() > 0) {
            networkChannels[i].name = channelName;
            
            // Cargar PSK del channel
            networkChannels[i].psk = preferences.getString(pskKey.c_str(), "");
            
            // Set active flag
            networkChannels[i].active = true;
            
            Serial.println("[LOAD] Channel " + String(i) + ": " + channelName);
        }
    }
    
    // Actualizar campos en DeviceConfig para coherencia
    if (activeChannelIndex >= 0 && activeChannelIndex < channelCount) {
        config.activeChannelIndex = activeChannelIndex;
        strncpy(config.activeChannelName, networkChannels[activeChannelIndex].name.c_str(), 
                MAX_CHANNEL_NAME_LENGTH);
        config.activeChannelName[MAX_CHANNEL_NAME_LENGTH] = '\0';
        config.hasActiveChannel = true;
        
        Serial.println("[LOAD] Active channel: " + String(config.activeChannelName));
    } else {
        config.activeChannelIndex = 0;
        strcpy(config.activeChannelName, "default");
        config.hasActiveChannel = false;
        Serial.println("[LOAD] No active channel found");
    }
}

/*
 * GUARDADO DE CONFIGURACIÓN EN EEPROM
 */
void ConfigManager::saveConfig() {
    preferences.putUChar("role", config.role);
    preferences.putUShort("deviceID", config.deviceID);
    preferences.putUShort("gpsInterval", config.gpsInterval);
    preferences.putUChar("maxHops", config.maxHops);
    preferences.putUChar("dataMode", config.dataMode);
    preferences.putUChar("region", config.region);
    preferences.putBool("configValid", config.configValid);
    preferences.putUChar("radioProfile", config.radioProfile);
    
    Serial.println("[OK] Configuración guardada exitosamente.");

    // Guardar cantidad total de channels
    preferences.putUChar(EEPROM_CHANNEL_COUNT_KEY, channelCount);
    
    // Guardar índice del channel activo
    preferences.putChar(EEPROM_ACTIVE_CHANNEL_KEY, activeChannelIndex);
    
    // Guardar cada channel individual
    for (int i = 0; i < channelCount && i < MAX_CHANNELS; i++) {
        // Construir keys dinámicamente: "ch_0_name", "ch_1_name", etc.
        String nameKey = String(EEPROM_CHANNEL_NAME_PREFIX) + String(i) + "_name";
        String pskKey = String(EEPROM_CHANNEL_PSK_PREFIX) + String(i);
        
        // Guardar nombre y PSK del channel
        preferences.putString(nameKey.c_str(), networkChannels[i].name);
        preferences.putString(pskKey.c_str(), networkChannels[i].psk);
        
        Serial.println("[SAVE] Channel " + String(i) + ": " + networkChannels[i].name);
    }
    
    // Limpiar channels que ya no existen (si channelCount se redujo)
    int previousCount = preferences.getUChar(EEPROM_CHANNEL_COUNT_KEY, 0);
    for (int i = channelCount; i < previousCount && i < MAX_CHANNELS; i++) {
        String nameKey = String(EEPROM_CHANNEL_NAME_PREFIX) + String(i) + "_name";
        String pskKey = String(EEPROM_CHANNEL_PSK_PREFIX) + String(i);
        
        preferences.remove(nameKey.c_str());
        preferences.remove(pskKey.c_str());
        
        Serial.println("[SAVE] Cleaned old channel " + String(i));
    }
    
    // Actualizar campos en DeviceConfig para coherencia
    if (activeChannelIndex >= 0 && activeChannelIndex < channelCount) {
        config.activeChannelIndex = activeChannelIndex;
        strncpy(config.activeChannelName, networkChannels[activeChannelIndex].name.c_str(), 
                MAX_CHANNEL_NAME_LENGTH);
        config.activeChannelName[MAX_CHANNEL_NAME_LENGTH] = '\0';
        config.hasActiveChannel = true;
    } else {
        config.activeChannelIndex = 0;
        strcpy(config.activeChannelName, "default");
        config.hasActiveChannel = false;
    }
    
    Serial.println("[SAVE] Saved " + String(channelCount) + " channels to EEPROM");
    
}

/*
 * MÉTODOS PARA GESTIÓN DE REGIÓN
 */
float ConfigManager::getFrequencyMHz() {
    switch (config.region) {
        case REGION_US: return FREQ_US_MHZ;
        case REGION_EU: return FREQ_EU_MHZ;
        case REGION_CH: return FREQ_CH_MHZ;
        case REGION_AS: return FREQ_AS_MHZ;
        case REGION_JP: return FREQ_JP_MHZ;
        default: return FREQ_US_MHZ;
    }
}

void ConfigManager::setDataMode(DataDisplayMode mode) {
    config.dataMode = mode;
    preferences.putUChar("dataMode", config.dataMode);
}

void ConfigManager::setGpsInterval(uint16_t interval) {
    if (interval >= 5 && interval <= 3600) {
        config.gpsInterval = interval;
    }
}

String ConfigManager::getCurrentDataModeString() {
    return getDataModeString(config.dataMode);
}

String ConfigManager::getRadioProfileName() {
    return radioProfileManager.getProfileName(config.radioProfile);
}

/*
 * MÉTODOS UTILITARIOS PRIVADOS
 */
void ConfigManager::printConfig() {
    Serial.println("\n=== CONFIGURACIÓN ACTUAL ===");
    Serial.println("Rol: " + getRoleString(config.role));
    Serial.println("Device ID: " + String(config.deviceID));
    Serial.println("Intervalo GPS: " + String(config.gpsInterval) + " segundos");
    Serial.println("Máximo saltos: " + String(config.maxHops));
    Serial.println("Modo de datos: " + getDataModeString(config.dataMode));
    Serial.println("Región LoRa: " + getRegionString(config.region) + " (" + String(getFrequencyMHz()) + " MHz)");
    Serial.println("Perfil LoRa: " + getRadioProfileName());
    Serial.println("============================");
}

void ConfigManager::printWelcome() {
    Serial.println("\n==================================================");
    Serial.println("    CUSTOM MESHTASTIC GPS TRACKER v" + String(config.version));
    Serial.println("    ESP32-S3 + LoRa SX1262");
    Serial.println("==================================================");
}

void ConfigManager::printPrompt() {
    Serial.print("config> ");
}

void ConfigManager::setDefaultConfig() {
    config.role = ROLE_NONE;
    config.deviceID = 0;
    config.gpsInterval = 30;
    config.maxHops = 3;
    config.dataMode = DATA_MODE_ADMIN;
    config.region = REGION_US;
    config.configValid = false;
    config.radioProfile = PROFILE_MESH_MAX_NODES;
    
    strncpy(config.version, FIRMWARE_VERSION, sizeof(config.version) - 1);
    config.version[sizeof(config.version) - 1] = '\0';
}