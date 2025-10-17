/*
 * CONFIG_MANAGER.CPP - Core del Sistema de Configuración
 * 
 * MODULARIZADO: Solo funciones core (constructor, begin, load/save, getters/setters)
 * Los comandos están en config_commands.cpp
 */

#include "config_manager.h"
#include "config_commands.h"

#include <cstring>

/*
 * DEFINICIONES Y CONSTANTES
 */

// Versión del firmware - se actualiza con cada release
#define FIRMWARE_VERSION "0.4.0"

// Tiempo de espera al inicio para entrar en modo configuración
#define STARTUP_CONFIG_WAIT 5000

#if !CONFIG_MANAGER_HAS_PREFERENCES
using namespace Adafruit_LittleFS_Namespace;

static constexpr const char* CONFIG_STORAGE_PATH = "/custodia.cfg";
static constexpr uint32_t CONFIG_STORAGE_MAGIC = 0x43555354; // 'CUST'
static constexpr uint16_t CONFIG_STORAGE_VERSION = 1;
static constexpr size_t STORAGE_NAME_CAPACITY = 21;   // 20 chars + null
static constexpr size_t STORAGE_PASS_CAPACITY = 33;   // 32 chars + null

struct PersistedNetwork {
    char name[STORAGE_NAME_CAPACITY];
    char password[STORAGE_PASS_CAPACITY];
    uint32_t hash;
    uint8_t active;
};

struct PersistedData {
    uint32_t magic;
    uint16_t version;
    DeviceConfig config;
    uint8_t networkCount;
    int8_t activeIndex;
    PersistedNetwork networks[MAX_NETWORKS];
};
#endif

/*
 * INSTANCIA GLOBAL
 */
ConfigManager configManager;

/*
 * CONSTRUCTOR
 */
ConfigManager::ConfigManager() {
    currentState = STATE_BOOT;

#if !CONFIG_MANAGER_HAS_PREFERENCES
    storageReady = false;
#endif

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
#if CONFIG_MANAGER_HAS_PREFERENCES
    // Inicializar sistema de preferencias con namespace "mesh-config"
    if (!preferences.begin("mesh-config", false)) {
        Serial.println("[ERROR] No se pudo inicializar sistema de preferencias");
        return;
    }
#else
    storageReady = InternalFS.begin();
    if (!storageReady) {
        Serial.println("[WARN] No se pudo montar InternalFS. La configuración no se almacenará de forma persistente.");
    } else {
        Serial.println("[INFO] Persistencia habilitada con InternalFS (LittleFS).");
    }
#endif

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
        if (radioProfileManager.isSupportedProfile(static_cast<uint8_t>(config.radioProfile))) {
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
    else if (input.startsWith("NETWORK_INFO")) {
        // Manejar tanto "NETWORK_INFO" como "NETWORK_INFO <nombre>"
        if (input.length() == 12) {
            handleNetworkInfo("");  // Sin parámetros
        } else {
            handleNetworkInfo(input.substring(13));  // Con nombre especifico
        }
    }
    else if (input == "NETWORK_STATUS") {
        handleNetworkStatus();
    }
    else if (input.startsWith("NETWORK_DELETE ")) {
        handleNetworkDelete(input.substring(15));
    }
    else if (input.startsWith("NETWORK_DELETE_CONFIRM ")) {
        handleNetworkDeleteConfirm(input.substring(23));
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
#if CONFIG_MANAGER_HAS_PREFERENCES
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
#else
    if (!storageReady || !loadFromStorage()) {
        setDefaultConfig();
        Serial.println("[INFO] Configuración por defecto cargada (sin datos persistidos).");
    }
#endif
    
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
#if CONFIG_MANAGER_HAS_PREFERENCES
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
#else
    if (saveToStorage()) {
        Serial.println("[OK] Configuración guardada exitosamente.");
    } else {
        Serial.println("[WARN] No se pudo guardar la configuración en el almacenamiento interno.");
    }
#endif
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
#if CONFIG_MANAGER_HAS_PREFERENCES
    preferences.putUChar("dataMode", config.dataMode);
#endif
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
#if CONFIG_MANAGER_HAS_PREFERENCES
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
#else
    if (!storageReady) {
        Serial.println("[WARN] Almacenamiento interno no disponible para guardar networks.");
        return;
    }

    if (saveToStorage()) {
        Serial.println("[Networks] Guardadas " + String(networkCount) + " networks en InternalFS.");
    } else {
        Serial.println("[WARN] Error al guardar networks en InternalFS.");
    }
#endif
}

// Cargar networks desde EEPROM
void ConfigManager::loadNetworks() {
#if CONFIG_MANAGER_HAS_PREFERENCES
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
#else
    if (!storageReady) {
        networkCount = 0;
        activeNetworkIndex = -1;
        return;
    }

    if (activeNetworkIndex >= networkCount) {
        activeNetworkIndex = networkCount > 0 ? 0 : -1;
    }

    for (int i = 0; i < networkCount; i++) {
        networks[i].active = (i == activeNetworkIndex);
    }

    if (networkCount > 0) {
        Serial.println("[Networks] Cargadas " + String(networkCount) + " networks desde InternalFS.");
        if (activeNetworkIndex >= 0) {
            Serial.println("[Networks] Network activa: " + networks[activeNetworkIndex].name);
        }
    }
#endif
}

// Verificar si el nombre está en la lista de nombres reservados
bool ConfigManager::isReservedNetworkName(String name) {
    name.trim();
    name.toUpperCase();
    
    // Lista de nombres reservados del sistema
    String reservedNames[] = {
        "CONFIG", "ADMIN", "DEBUG", "SYSTEM", "DEVICE", 
        "LORA", "MESH", "NETWORK", "DEFAULT", "TEST",
        "GPS", "TRACKER", "REPEATER", "RECEIVER", "END_NODE_REPEATER"
    };
    
    int numReserved = sizeof(reservedNames) / sizeof(reservedNames[0]);
    
    for (int i = 0; i < numReserved; i++) {
        if (name.equals(reservedNames[i])) {
            return true;  // Es un nombre reservado
        }
    }
    
    return false;  // No es reservado
}

// Verificar que la password tenga al menos un número y una letra
bool ConfigManager::hasNumberAndLetter(String password) {
    bool hasNumber = false;
    bool hasLetter = false;
    
    for (int i = 0; i < password.length(); i++) {
        char c = password.charAt(i);
        if (isDigit(c)) {
            hasNumber = true;
        }
        if (isAlpha(c)) {
            hasLetter = true;
        }
        
        // Si ya encontramos ambos, podemos salir temprano
        if (hasNumber && hasLetter) {
            return true;
        }
    }
    
    return (hasNumber && hasLetter);
}

// Validar que la password cumple criterios de seguridad
bool ConfigManager::isPasswordSecure(String password) {
    // Verificar longitud básica (ya se hace en isValidPassword)
    if (password.length() < 8 || password.length() > 32) {
        return false;
    }
    
    // Debe tener al menos un número y una letra
    if (!hasNumberAndLetter(password)) {
        return false;
    }
    
    // No puede ser una secuencia simple
    if (password.equals("12345678") || password.equals("ABCDEFGH") || 
        password.equals("PASSWORD") || password.equals("QWERTYUI")) {
        return false;
    }
    
    return true;
}

// Validación completa de nombre de network con mensaje de error detallado
String ConfigManager::validateNetworkNameAdvanced(String name, String& errorMsg) {
    // Limpiar y normalizar
    name.trim();
    String originalName = name;
    name.toUpperCase();
    
    // Verificar longitud
    if (name.length() < 3) {
        errorMsg = "Nombre muy corto. Mínimo 3 caracteres.";
        return "";
    }
    if (name.length() > 20) {
        errorMsg = "Nombre muy largo. Máximo 20 caracteres.";
        return "";
    }
    
    // Verificar caracteres válidos
    for (int i = 0; i < name.length(); i++) {
        char c = name.charAt(i);
        if (!isAlphaNumeric(c) && c != '_' && c != '-') {
            errorMsg = "Carácter inválido '" + String(c) + "'. Use solo letras, números, guiones y underscore.";
            return "";
        }
    }
    
    // No puede empezar o terminar con guión/underscore
    if (name.charAt(0) == '-' || name.charAt(0) == '_' || 
        name.charAt(name.length()-1) == '-' || name.charAt(name.length()-1) == '_') {
        errorMsg = "No puede empezar o terminar con guión o underscore.";
        return "";
    }
    
    // Verificar nombres reservados
    if (isReservedNetworkName(name)) {
        errorMsg = "Nombre reservado del sistema. Use otro nombre.";
        return "";
    }
    
    // Verificar duplicados
    if (findNetworkByName(name) >= 0) {
        errorMsg = "Ya existe una network con ese nombre.";
        return "";
    }
    
    // Todo válido
    errorMsg = "";
    return name;
}

// Validación completa de password con mensaje de error detallado
String ConfigManager::validatePasswordAdvanced(String password, String networkName, String& errorMsg) {
    // Limpiar y normalizar
    password.trim();
    password.toUpperCase();
    networkName.toUpperCase();
    
    // Verificar longitud
    if (password.length() < 8) {
        errorMsg = "Password muy corta. Mínimo 8 caracteres.";
        return "";
    }
    if (password.length() > 32) {
        errorMsg = "Password muy larga. Máximo 32 caracteres.";
        return "";
    }
    
    // Verificar caracteres válidos (solo alfanuméricos por ahora)
    for (int i = 0; i < password.length(); i++) {
        char c = password.charAt(i);
        if (!isAlphaNumeric(c)) {
            errorMsg = "Solo se permiten letras y números en la password.";
            return "";
        }
    }
    
    // Verificar criterios de seguridad
    if (!hasNumberAndLetter(password)) {
        errorMsg = "Password debe tener al menos una letra y un número.";
        return "";
    }
    
    // No puede ser igual al nombre de la network
    if (password.equals(networkName)) {
        errorMsg = "Password no puede ser igual al nombre de la network.";
        return "";
    }
    
    // Verificar que no sea una password débil
    if (!isPasswordSecure(password)) {
        errorMsg = "Password demasiado simple. Evite secuencias obvias.";
        return "";
    }
    
    // Todo válido
    errorMsg = "";
    return password;
}

// Verificar si se puede eliminar una network específica
bool ConfigManager::canDeleteNetwork(String name, String& errorMsg) {
    name.trim();
    name.toUpperCase();
    
    // Verificar que la network existe
    int networkIndex = findNetworkByName(name);
    if (networkIndex < 0) {
        errorMsg = "Network '" + name + "' no existe.";
        return false;
    }
    
    // No se puede eliminar si es la única network
    if (networkCount <= 1) {
        errorMsg = "No se puede eliminar la única network. Cree otra primero.";
        return false;
    }
    
    // Todo válido para eliminar
    errorMsg = "";
    return true;
}

// Calcular memoria EEPROM usada por networks (aproximación)
uint16_t ConfigManager::getEEPROMUsageBytes() {
#if CONFIG_MANAGER_HAS_PREFERENCES
    uint16_t totalBytes = 0;
    
    // Bytes fijos por network metadata
    totalBytes += sizeof(networkCount);      // Contador de networks
    totalBytes += sizeof(activeNetworkIndex); // Índice activo
    
    // Bytes por cada network guardada
    for (int i = 0; i < networkCount; i++) {
        // Cada network usa: name + password + hash
        totalBytes += networks[i].name.length() + 1;     // +1 por null terminator
        totalBytes += networks[i].password.length() + 1; // +1 por null terminator  
        totalBytes += sizeof(uint32_t);                  // hash
    }
    
    // Overhead estimado de las keys EEPROM (aproximación)
    totalBytes += networkCount * 30; // ~30 bytes promedio por set de keys
    
    return totalBytes;
#else
    return 0;
#endif
}

// Calcular memoria EEPROM disponible (estimación conservadora)
uint16_t ConfigManager::getAvailableEEPROMBytes() {
#if CONFIG_MANAGER_HAS_PREFERENCES
    // ESP32 Preferences tiene ~4KB disponibles típicamente
    // Reservamos espacio para la configuración general del dispositivo
    const uint16_t TOTAL_EEPROM_SIZE = 4096;
    const uint16_t RESERVED_FOR_CONFIG = 512;  // Para config general del dispositivo
    const uint16_t SAFETY_MARGIN = 256;       // Margen de seguridad
    
    uint16_t usedBytes = getEEPROMUsageBytes();
    uint16_t availableForNetworks = TOTAL_EEPROM_SIZE - RESERVED_FOR_CONFIG - SAFETY_MARGIN;
    
    if (usedBytes >= availableForNetworks) {
        return 0;  // No hay espacio disponible
    }
    
    return availableForNetworks - usedBytes;
#else
    return 0;
#endif
}

void ConfigManager::handleNetworkStatus() {
    Serial.println("========================================");
    Serial.println("      ESTADO SISTEMA NETWORKS");
    Serial.println("========================================");
    
    // Estadísticas básicas
    Serial.println("Networks guardadas: " + String(networkCount) + "/" + String(MAX_NETWORKS));
    
    if (networkCount == 0) {
        Serial.println("Estado:           SIN NETWORKS");
        Serial.println("[INFO] Use 'NETWORK_CREATE <nombre>' para crear la primera network.");
        Serial.println("========================================");
        return;
    }
    
    // Network activa
    if (hasActiveNetwork()) {
        SimpleNetwork* active = getActiveNetwork();
        Serial.println("Network activa:   " + active->name);
        Serial.println("Hash activo:      " + String(active->hash, HEX));
    } else {
        Serial.println("Network activa:   NINGUNA");
        Serial.println("[WARNING] No hay network activa!");
    }
    
    // Análisis de seguridad
    int secureNetworks = 0;
    int weakPasswords = 0;
    
    for (int i = 0; i < networkCount; i++) {
        if (isPasswordSecure(networks[i].password)) {
            secureNetworks++;
        } else {
            weakPasswords++;
        }
    }
    
    Serial.println("Networks seguras: " + String(secureNetworks) + "/" + String(networkCount));
    if (weakPasswords > 0) {
        Serial.println("Passwords débiles: " + String(weakPasswords) + " [WARNING]");
    }
    
    // Uso de memoria EEPROM
    uint16_t usedBytes = getEEPROMUsageBytes();
    uint16_t availableBytes = getAvailableEEPROMBytes();
    uint16_t totalNetworkSpace = usedBytes + availableBytes;
    
    Serial.println("----------------------------------------");
    Serial.println("Memoria EEPROM (networks):");
    Serial.println("  Usada:        " + String(usedBytes) + " bytes");
    Serial.println("  Disponible:   " + String(availableBytes) + " bytes");
    Serial.println("  Total:        " + String(totalNetworkSpace) + " bytes");
    
    if (totalNetworkSpace > 0) {
        // Calcular porcentaje usado
        float percentUsed = (float)usedBytes / (float)totalNetworkSpace * 100.0;
        Serial.println("  Uso:          " + String(percentUsed, 1) + "%");
        
        // Advertencias de memoria
        if (percentUsed > 90.0) {
            Serial.println("  [CRITICAL] Memoria crítica!");
        } else if (percentUsed > 80.0) {
            Serial.println("  [WARNING] Memoria casi llena!");
        }
    } else {
        Serial.println("  Uso:          N/A (sin almacenamiento persistente)");
    }
    
    // Capacidad estimada
    int estimatedCapacity = availableBytes / 40; // ~40 bytes promedio por network
    Serial.println("  Capacidad est: +" + String(estimatedCapacity) + " networks más");
    
    Serial.println("========================================");
    Serial.println("Sistema:          " + String(config.configValid ? "CONFIGURADO" : "SIN CONFIGURAR"));
    Serial.println("Dispositivo ID:   " + String(config.deviceID));
    Serial.println("Rol:              " + getRoleString(config.role));
    Serial.println("========================================");
}

// ========================================================

/*
 * MÉTODOS UTILITARIOS PRIVADOS
 */

void ConfigManager::printConfig() {
    Serial.println("\n=== CONFIGURACIÓN ACTUAL ===");
    Serial.println("Rol: " + getRoleString(config.role));
    Serial.println("Device ID: " + String(config.deviceID));
    Serial.println("Región LoRa: " + getRegionString(config.region) + " (" + String(getFrequencyMHz()) + " MHz)");
    Serial.println("Perfil LoRa: " + getRadioProfileName());

    if (config.role != ROLE_END_NODE_REPEATER) {
        Serial.println("Intervalo GPS: " + String(config.gpsInterval) + " segundos");
        Serial.println("Máximo saltos: " + String(config.maxHops));
        Serial.println("Modo de datos: " + getDataModeString(config.dataMode));
    }

    Serial.println("============================");
}

void ConfigManager::printWelcome() {
    Serial.println("\n==================================================");
    Serial.println("    CUSTOM MESHTASTIC GPS TRACKER v" + String(config.version));
#if defined(ARDUINO_ARCH_ESP32)
    Serial.println("    ESP32-S3 + LoRa SX1262");
#elif defined(NRF52_SERIES)
    Serial.println("    nRF52840 + LoRa SX1262");
#else
    Serial.println("    Plataforma: Desconocida");
#endif
    Serial.println("==================================================");
}

#if !CONFIG_MANAGER_HAS_PREFERENCES

bool ConfigManager::loadFromStorage() {
    if (!storageReady) {
        return false;
    }

    File file(InternalFS);
    if (!file.open(CONFIG_STORAGE_PATH, FILE_O_READ)) {
        return false;
    }

    PersistedData data = {};
    size_t readLen = file.read(&data, sizeof(data));
    file.close();

    if (readLen != sizeof(data)) {
        Serial.println("[WARN] Archivo de configuración incompleto, usando valores por defecto.");
        return false;
    }

    if (data.magic != CONFIG_STORAGE_MAGIC || data.version != CONFIG_STORAGE_VERSION) {
        Serial.println("[WARN] Versión de configuración incompatible, se ignorará el archivo.");
        return false;
    }

    config = data.config;

    uint8_t storedCount = data.networkCount;
    if (storedCount > MAX_NETWORKS) {
        storedCount = MAX_NETWORKS;
    }

    networkCount = storedCount;
    activeNetworkIndex = data.activeIndex;

    if (activeNetworkIndex >= networkCount) {
        activeNetworkIndex = networkCount > 0 ? 0 : -1;
    }

    for (int i = 0; i < MAX_NETWORKS; i++) {
        networks[i] = SimpleNetwork();
    }

    for (int i = 0; i < networkCount; i++) {
        networks[i].name = String(data.networks[i].name);
        networks[i].password = String(data.networks[i].password);
        networks[i].hash = data.networks[i].hash;
        networks[i].active = (i == activeNetworkIndex);
    }

    return true;
}

bool ConfigManager::saveToStorage() {
    if (!storageReady) {
        return false;
    }

    PersistedData data = {};
    data.magic = CONFIG_STORAGE_MAGIC;
    data.version = CONFIG_STORAGE_VERSION;
    data.config = config;

    uint8_t storedCount = networkCount;
    if (storedCount > MAX_NETWORKS) {
        storedCount = MAX_NETWORKS;
    }

    data.networkCount = storedCount;
    data.activeIndex = activeNetworkIndex;

    for (uint8_t i = 0; i < storedCount; i++) {
        strncpy(data.networks[i].name, networks[i].name.c_str(), STORAGE_NAME_CAPACITY - 1);
        data.networks[i].name[STORAGE_NAME_CAPACITY - 1] = '\0';
        strncpy(data.networks[i].password, networks[i].password.c_str(), STORAGE_PASS_CAPACITY - 1);
        data.networks[i].password[STORAGE_PASS_CAPACITY - 1] = '\0';
        data.networks[i].hash = networks[i].hash;
        data.networks[i].active = (i == activeNetworkIndex) ? 1 : 0;
    }

    // Limpiar entradas restantes
    for (uint8_t i = storedCount; i < MAX_NETWORKS; i++) {
        memset(&data.networks[i], 0, sizeof(PersistedNetwork));
    }

    // Para evitar corrupción en LittleFS, es más seguro eliminar y recrear el archivo.
    InternalFS.remove(CONFIG_STORAGE_PATH);

    File file(InternalFS);
    if (!file.open(CONFIG_STORAGE_PATH, FILE_O_WRITE)) {
        return false;
    }

    size_t written = file.write(reinterpret_cast<const uint8_t*>(&data), sizeof(data));
    file.flush();
    file.close();

    return written == sizeof(data);
}

void ConfigManager::clearStorage() {
    if (!storageReady) {
        return;
    }

    if (InternalFS.exists(CONFIG_STORAGE_PATH)) {
        InternalFS.remove(CONFIG_STORAGE_PATH);
    }
}

#endif

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
