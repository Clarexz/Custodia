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
    
    // ========== NUEVOS COMANDOS DE RADIO PROFILES ==========
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
    // ======================================================
    
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
    // Cargar cada parámetro con valores por defecto si no existen
    config.role = (DeviceRole)preferences.getUChar("role", ROLE_NONE);
    config.deviceID = preferences.getUShort("deviceID", 0);
    config.gpsInterval = preferences.getUShort("gpsInterval", 30);
    config.maxHops = preferences.getUChar("maxHops", 3);
    config.dataMode = (DataDisplayMode)preferences.getUChar("dataMode", DATA_MODE_ADMIN);
    config.region = (LoRaRegion)preferences.getUChar("region", REGION_US);
    config.configValid = preferences.getBool("configValid", false);
    
    // NUEVO: Cargar Radio Profile
    config.radioProfile = (RadioProfile)preferences.getUChar("radioProfile", PROFILE_MESH_MAX_NODES);
    
    // Establecer versión del firmware actual
    strncpy(config.version, FIRMWARE_VERSION, sizeof(config.version) - 1);
    config.version[sizeof(config.version) - 1] = '\0';
    
    // Validación adicional
    if (config.role == ROLE_NONE || config.deviceID == 0) {
        config.configValid = false;
    }
}

/*
 * GUARDADO DE CONFIGURACIÓN EN EEPROM
 */
void ConfigManager::saveConfig() {
    // Guardar cada parámetro en su clave específica
    preferences.putUChar("role", config.role);
    preferences.putUShort("deviceID", config.deviceID);
    preferences.putUShort("gpsInterval", config.gpsInterval);
    preferences.putUChar("maxHops", config.maxHops);
    preferences.putUChar("dataMode", config.dataMode);
    preferences.putUChar("region", config.region);
    preferences.putBool("configValid", config.configValid);
    
    // NUEVO: Guardar Radio Profile
    preferences.putUChar("radioProfile", config.radioProfile);
    
    Serial.println("[OK] Configuración guardada exitosamente.");
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
        default: return FREQ_US_MHZ; // Default US
    }
}

void ConfigManager::setDataMode(DataDisplayMode mode) {
    config.dataMode = mode;
    preferences.putUChar("dataMode", config.dataMode);
}

void ConfigManager::setGpsInterval(uint16_t interval) {
    if (interval >= 5 && interval <= 3600) {
        config.gpsInterval = interval;
        // Para prototipo no guardamos automáticamente en EEPROM
        // preferences.putUShort("gpsInterval", config.gpsInterval);
    }
}

String ConfigManager::getCurrentDataModeString() {
    return getDataModeString(config.dataMode);
}

// ========== NUEVOS MÉTODOS PARA RADIO PROFILES ==========

String ConfigManager::getRadioProfileName() {
    return radioProfileManager.getProfileName(config.radioProfile);
}

// ========================================================

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
    
    // NUEVO: Mostrar Radio Profile
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
    
    // NUEVO: Radio Profile por defecto
    config.radioProfile = PROFILE_MESH_MAX_NODES;
    
    strncpy(config.version, FIRMWARE_VERSION, sizeof(config.version) - 1);
    config.version[sizeof(config.version) - 1] = '\0';
}