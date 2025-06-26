/*
 * CONFIG.CPP - Implementación del Sistema de Configuración
 * 
 * Este archivo implementa toda la funcionalidad del sistema de configuración
 * para el Custom Meshtastic GPS Tracker. Maneja comandos seriales, validación
 * de parámetros, persistencia en EEPROM y control de estados del sistema.
 */

#include "config.h"
#include <WiFi.h>

/*
 * DEFINICIONES Y CONSTANTES
 */

// Versión del firmware - se actualiza con cada release
#define FIRMWARE_VERSION "2.1.0"

// Timeout en milisegundos para confirmaciones de usuario
#define CONFIRMATION_TIMEOUT 10000

// Tiempo de espera al inicio para entrar en modo configuración
#define STARTUP_CONFIG_WAIT 5000

/*
 * INSTANCIA GLOBAL
 * 
 * Se define aquí la instancia global del ConfigManager declarada en config.h
 */
ConfigManager configManager;

/*
 * CONSTRUCTOR
 * 
 * Inicializa el ConfigManager en estado de arranque
 */
ConfigManager::ConfigManager() {
    currentState = STATE_BOOT;
}

/*
 * MÉTODO PRINCIPAL DE INICIALIZACIÓN
 * 
 * Este método se ejecuta una vez al arrancar el dispositivo y:
 * 1. Inicializa el sistema de preferencias (EEPROM)
 * 2. Carga configuración existente
 * 3. Determina si entrar en modo configuración u operativo
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
        Serial.println("[INFO] Iniciando modo operativo...");
    }
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
    config.dataMode = (DataDisplayMode)preferences.getUChar("dataMode", DATA_MODE_ADMIN); // Default ADMIN
    config.configValid = preferences.getBool("configValid", false);
    
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
    preferences.putUChar("dataMode", config.dataMode); // Guardar modo de datos
    preferences.putBool("configValid", config.configValid);
    
    Serial.println("[OK] Configuración guardada exitosamente.");
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
     * PARSER DE COMANDOS - ACTUALIZADO CON CONFIG_DATA_MODE
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
    else if (input.startsWith("MODE ")) {
        // NUEVO: Comando para cambiar modo durante operación
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
 * MANEJADORES DE COMANDOS EXISTENTES
 */

void ConfigManager::handleConfigRole(String value) {
    value.trim();
    
    if (value == "TRACKER") {
        config.role = ROLE_TRACKER;
        Serial.println("[OK] Rol configurado: TRACKER");
    }
    else if (value == "REPEATER") {
        config.role = ROLE_REPEATER;
        Serial.println("[OK] Rol configurado: REPEATER");
    }
    else if (value == "RECEIVER") {
        config.role = ROLE_RECEIVER;
        Serial.println("[OK] Rol configurado: RECEIVER");
    }
    else {
        Serial.println("[ERROR] Rol inválido. Use: TRACKER, REPEATER, o RECEIVER");
        return;
    }
    
    // Actualizar validez de configuración
    if (config.deviceID > 0) {
        config.configValid = true;
    }
}

void ConfigManager::handleConfigDeviceID(String value) {
    int id = value.toInt();
    
    if (id >= 1 && id <= 999) {
        config.deviceID = id;
        Serial.println("[OK] Device ID configurado: " + String(id));
        
        if (config.role != ROLE_NONE) {
            config.configValid = true;
        }
    } else {
        Serial.println("[ERROR] Device ID inválido. Use un número entre 1 y 999.");
    }
}

void ConfigManager::handleConfigGpsInterval(String value) {
    int interval = value.toInt();
    
    if (interval >= 5 && interval <= 3600) {
        config.gpsInterval = interval;
        Serial.println("[OK] Intervalo GPS configurado: " + String(interval) + " segundos");
    } else {
        Serial.println("[ERROR] Intervalo inválido. Use un valor entre 5 y 3600 segundos.");
    }
}

void ConfigManager::handleConfigMaxHops(String value) {
    int hops = value.toInt();
    
    if (hops >= 1 && hops <= 10) {
        config.maxHops = hops;
        Serial.println("[OK] Máximo de saltos configurado: " + String(hops));
    } else {
        Serial.println("[ERROR] Número de saltos inválido. Use un valor entre 1 y 10.");
    }
}

/*
 * ACTUALIZADO: MANEJADOR PARA CONFIG_DATA_MODE
 */
void ConfigManager::handleConfigDataMode(String value) {
    value.trim();
    
    if (value == "SIMPLE") {
        config.dataMode = DATA_MODE_SIMPLE;
        Serial.println("[OK] Modo de datos configurado: SIMPLE");
        Serial.println("[INFO] Se mostrará solo: [deviceID, latitude, longitude, batteryvoltage, timestamp]");
    }
    else if (value == "ADMIN") {
        config.dataMode = DATA_MODE_ADMIN;
        Serial.println("[OK] Modo de datos configurado: ADMIN");
        Serial.println("[INFO] Se mostrará información completa de mesh y estadísticas");
    }
    else {
        Serial.println("[ERROR] Modo inválido. Use: SIMPLE o ADMIN");
        Serial.println("[INFO] SIMPLE: Solo packet básico");
        Serial.println("[INFO] ADMIN: Información completa de mesh");
        return;
    }
}

/*
 * NUEVO: MANEJADOR PARA CAMBIO DE MODO DURANTE OPERACIÓN
 */
void ConfigManager::handleModeChange(String value) {
    value.trim();
    
    if (value == "SIMPLE") {
        config.dataMode = DATA_MODE_SIMPLE;
        Serial.println("[OK] Modo cambiado a: SIMPLE");
        Serial.println("[INFO] Mostrando solo packets básicos");
        // Auto-guardar cambio
        preferences.putUChar("dataMode", config.dataMode);
    }
    else if (value == "ADMIN") {
        config.dataMode = DATA_MODE_ADMIN;
        Serial.println("[OK] Modo cambiado a: ADMIN");
        Serial.println("[INFO] Mostrando información completa");
        // Auto-guardar cambio
        preferences.putUChar("dataMode", config.dataMode);
    }
    else {
        Serial.println("[ERROR] Modo inválido. Use: MODE SIMPLE o MODE ADMIN");
        Serial.println("[INFO] Modo actual: " + getCurrentDataModeString());
        return;
    }
}

void ConfigManager::handleConfigSave() {
    if (!config.configValid) {
        Serial.println("[ERROR] Configuración inválida. Configure ROLE y DEVICE_ID primero.");
        return;
    }
    
    saveConfig();
}

void ConfigManager::handleConfigReset() {
    Serial.print("[WARNING] ¿Está seguro que desea resetear la configuración? (Y/N): ");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        if (Serial.available()) {
            String confirm = Serial.readStringUntil('\n');
            confirm.trim();
            confirm.toUpperCase();
            Serial.println(confirm);
            
            if (confirm == "Y" || confirm == "YES") {
                setDefaultConfig();
                preferences.clear();
                Serial.println("[OK] Configuración reseteada. Reinicie el dispositivo.");
                return;
            } else {
                Serial.println("[INFO] Reset cancelado.");
                return;
            }
        }
        delay(100);
    }
    Serial.println("\n[INFO] Timeout. Reset cancelado.");
}

void ConfigManager::handleInfo() {
    Serial.println("\n=== INFORMACIÓN DEL DISPOSITIVO ===");
    Serial.println("Firmware: Custom Meshtastic v" + String(config.version));
    Serial.println("Chip: ESP32-S3");
    Serial.println("MAC: " + WiFi.macAddress());
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("====================================");
}

void ConfigManager::handleStatus() {
    Serial.println("\n=== STATUS DEL SISTEMA ===");
    Serial.println("Estado: " + getStateString(currentState));
    Serial.println("Configuración válida: " + String(config.configValid ? "SÍ" : "NO"));
    if (config.configValid) {
        printConfig();
    }
    Serial.println("==========================");
}

void ConfigManager::handleHelp() {
    Serial.println("\n=== COMANDOS DISPONIBLES ===");
    Serial.println("CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>  - Configurar rol del dispositivo");
    Serial.println("CONFIG_DEVICE_ID <1-999>                 - Configurar ID único");
    Serial.println("CONFIG_GPS_INTERVAL <5-3600>             - Intervalo GPS en segundos");
    Serial.println("CONFIG_MAX_HOPS <1-10>                   - Máximo saltos en mesh");
    Serial.println("CONFIG_DATA_MODE <SIMPLE|ADMIN>          - Modo de visualización de datos");
    Serial.println("CONFIG_SAVE                              - Guardar configuración");
    Serial.println("CONFIG_RESET                             - Resetear configuración");
    Serial.println("INFO                                     - Información del dispositivo");
    Serial.println("STATUS                                   - Estado actual del sistema");
    Serial.println("START                                    - Iniciar modo operativo");
    Serial.println("HELP                                     - Mostrar esta ayuda");
    Serial.println("");
    Serial.println("=== COMANDOS DURANTE OPERACIÓN ===");
    Serial.println("MODE SIMPLE                              - Cambiar a vista simple");
    Serial.println("MODE ADMIN                               - Cambiar a vista completa");
    Serial.println("");
    Serial.println("=== MODOS DE DATOS ===");
    Serial.println("SIMPLE: Solo packet [deviceID, lat, lon, battery, timestamp]");
    Serial.println("ADMIN:  Información completa de mesh y estadísticas");
    Serial.println("============================");
}

/*
 * NUEVOS MÉTODOS PARA GESTIÓN DE MODO
 */

void ConfigManager::setDataMode(DataDisplayMode mode) {
    config.dataMode = mode;
    preferences.putUChar("dataMode", config.dataMode);
}

String ConfigManager::getCurrentDataModeString() {
    return getDataModeString(config.dataMode);
}

/*
 * MÉTODOS UTILITARIOS
 */

void ConfigManager::printConfig() {
    Serial.println("\n=== CONFIGURACIÓN ACTUAL ===");
    Serial.println("Rol: " + getRoleString(config.role));
    Serial.println("Device ID: " + String(config.deviceID));
    Serial.println("Intervalo GPS: " + String(config.gpsInterval) + " segundos");
    Serial.println("Máximo saltos: " + String(config.maxHops));
    Serial.println("Modo de datos: " + getDataModeString(config.dataMode));
    Serial.println("============================");
}

void ConfigManager::printWelcome() {
    Serial.println("\n==================================================");
    Serial.println("    CUSTOM MESHTASTIC GPS TRACKER v" + String(config.version));
    Serial.println("    Basado en ESP32-S3 + LoRa SX1262");
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
    config.dataMode = DATA_MODE_ADMIN;  // Default ADMIN
    config.configValid = false;
    strncpy(config.version, FIRMWARE_VERSION, sizeof(config.version) - 1);
    config.version[sizeof(config.version) - 1] = '\0';
}

String ConfigManager::getRoleString(DeviceRole role) {
    switch (role) {
        case ROLE_TRACKER: return "TRACKER";
        case ROLE_REPEATER: return "REPEATER";
        case ROLE_RECEIVER: return "RECEIVER";
        default: return "NO CONFIGURADO";
    }
}

String ConfigManager::getStateString(SystemState state) {
    switch (state) {
        case STATE_BOOT: return "INICIANDO";
        case STATE_CONFIG_MODE: return "MODO CONFIGURACIÓN";
        case STATE_RUNNING: return "OPERATIVO";
        case STATE_SLEEP: return "SUSPENDIDO";
        default: return "DESCONOCIDO";
    }
}

String ConfigManager::getDataModeString(DataDisplayMode mode) {
    switch (mode) {
        case DATA_MODE_SIMPLE: return "SIMPLE";
        case DATA_MODE_ADMIN: return "ADMIN";
        default: return "DESCONOCIDO";
    }
}