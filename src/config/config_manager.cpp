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

    // ===== INICIALIZAR VARIABLES DE NETWORKS =====
    networkCount = 0;
    activeNetworkIndex = -1;  // -1 significa "ninguna network activa"
    
    // Inicializar array de networks (opcional, pero buena práctica)
    for (int i = 0; i < MAX_NETWORKS; i++) {
        networks[i] = SimpleNetwork();  // Constructor por defecto
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

    // Cargar networks desde EEPROM
    loadNetworks();
    
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
    // ========== NUEVOS COMANDOS DE NETWORKS ==========
    else if (input == "NETWORK_LIST") {
        handleNetworkList();
    }
    else if (input.startsWith("NETWORK_CREATE ")) {
        handleNetworkCreate(input.substring(15));
    }
    else if (input.startsWith("NETWORK_JOIN ")) {
        handleNetworkJoin(input.substring(13));
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

    //  GUARDAR NETWORKS 
    saveNetworks();
    
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

/*
 * ===== IMPLEMENTACIÓN DE MÉTODOS PÚBLICOS PARA NETWORKS =====
 */

// Obtener network activa actual
SimpleNetwork* ConfigManager::getActiveNetwork() {
    if (activeNetworkIndex >= 0 && activeNetworkIndex < networkCount) {
        return &networks[activeNetworkIndex];
    }
    return nullptr;  // No hay network activa
}

// Obtener hash de la network activa (para packets LoRa)
uint32_t ConfigManager::getActiveNetworkHash() {
    SimpleNetwork* activeNet = getActiveNetwork();
    if (activeNet != nullptr) {
        return activeNet->hash;
    }
    return 0;  // Hash por defecto si no hay network activa
}

// Verificar si hay una network activa
bool ConfigManager::hasActiveNetwork() {
    return (activeNetworkIndex >= 0 && activeNetworkIndex < networkCount);
}

// Obtener network por índice (para listar)
SimpleNetwork* ConfigManager::getNetwork(uint8_t index) {
    if (index < networkCount) {
        return &networks[index];
    }
    return nullptr;  // Índice fuera de rango
}

/*
 * ===== MÉTODOS PRIVADOS DE VALIDACIÓN =====
 */

// Validar nombre de network
bool ConfigManager::isValidNetworkName(String name) {
    // Eliminar espacios y convertir a uppercase
    name.trim();
    name.toUpperCase();
    
    // Verificar longitud (3-20 caracteres)
    if (name.length() < 3 || name.length() > 20) {
        return false;
    }
    
    // Verificar que solo contenga caracteres alfanuméricos y guiones
    for (int i = 0; i < name.length(); i++) {
        char c = name.charAt(i);
        if (!isAlphaNumeric(c) && c != '_' && c != '-') {
            return false;
        }
    }
    
    return true;
}

// Validar password
bool ConfigManager::isValidPassword(String password) {
    // Eliminar espacios y convertir a uppercase
    password.trim();
    password.toUpperCase();
    
    // Verificar longitud (8-32 caracteres)
    if (password.length() < 8 || password.length() > 32) {
        return false;
    }
    
    // Verificar que solo contenga caracteres alfanuméricos
    for (int i = 0; i < password.length(); i++) {
        char c = password.charAt(i);
        if (!isAlphaNumeric(c)) {
            return false;
        }
    }
    
    return true;
}

// Buscar network por nombre (retorna índice o -1 si no se encuentra)
int ConfigManager::findNetworkByName(String name) {
    name.trim();
    name.toUpperCase();
    
    for (int i = 0; i < networkCount; i++) {
        if (networks[i].name.equals(name)) {
            return i;
        }
    }
    
    return -1;  // No encontrada
}

// Generar password aleatoria (8 caracteres alfanuméricos)
String ConfigManager::generateRandomPassword() {
    String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String password = "";
    
    // Usar millis() como semilla para mejor aleatoriedad
    randomSeed(millis());
    
    for (int i = 0; i < 8; i++) {
        password += chars.charAt(random(chars.length()));
    }
    
    return password;
}
// Guardar networks a EEPROM
void ConfigManager::saveNetworks() {
    // Guardar contador de networks
    preferences.putUChar(NETWORK_COUNT_KEY, networkCount);
    
    // Guardar índice de network activa
    preferences.putChar(ACTIVE_NETWORK_KEY, activeNetworkIndex);
    
    // Guardar cada network
    for (int i = 0; i < networkCount; i++) {
        String nameKey = String(NETWORK_NAME_PREFIX) + String(i);
        String passKey = String(NETWORK_PASS_PREFIX) + String(i);
        String hashKey = String(NETWORK_HASH_PREFIX) + String(i);
        
        preferences.putString(nameKey.c_str(), networks[i].name);
        preferences.putString(passKey.c_str(), networks[i].password);
        preferences.putUInt(hashKey.c_str(), networks[i].hash);
    }
    
    Serial.println("[Networks] Guardadas " + String(networkCount) + " networks en EEPROM.");
}

// Cargar networks desde EEPROM
void ConfigManager::loadNetworks() {
    // Cargar contador de networks
    networkCount = preferences.getUChar(NETWORK_COUNT_KEY, 0);
    
    // Cargar índice de network activa
    activeNetworkIndex = preferences.getChar(ACTIVE_NETWORK_KEY, -1);
    
    // Validar datos cargados
    if (networkCount > MAX_NETWORKS) {
        Serial.println("[Networks] ERROR: Contador inválido, reseteando networks.");
        networkCount = 0;
        activeNetworkIndex = -1;
        return;
    }
    
    if (activeNetworkIndex >= networkCount) {
        Serial.println("[Networks] WARNING: Índice activo inválido, corrigiendo.");
        activeNetworkIndex = networkCount > 0 ? 0 : -1;
    }
    
    // Cargar cada network
    for (int i = 0; i < networkCount; i++) {
        String nameKey = String(NETWORK_NAME_PREFIX) + String(i);
        String passKey = String(NETWORK_PASS_PREFIX) + String(i);
        String hashKey = String(NETWORK_HASH_PREFIX) + String(i);
        
        networks[i].name = preferences.getString(nameKey.c_str(), "");
        networks[i].password = preferences.getString(passKey.c_str(), "");
        networks[i].hash = preferences.getUInt(hashKey.c_str(), 0);
        networks[i].active = (i == activeNetworkIndex);
        
        // Validar que la network cargada es válida
        if (networks[i].name.length() == 0 || networks[i].password.length() == 0) {
            Serial.println("[Networks] ERROR: Network " + String(i) + " corrupta, reseteando.");
            networkCount = 0;
            activeNetworkIndex = -1;
            return;
        }
    }
    
    if (networkCount > 0) {
        Serial.println("[Networks] Cargadas " + String(networkCount) + " networks desde EEPROM.");
        if (activeNetworkIndex >= 0) {
            Serial.println("[Networks] Network activa: " + networks[activeNetworkIndex].name);
        }
    }
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