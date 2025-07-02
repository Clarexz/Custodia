/*
 * CONFIG_COMMANDS.CPP - Implementación de Manejadores de Comandos
 * 
 * MODULARIZADO: Todos los handle*() methods extraídos de config.cpp
 * para separar la lógica de comandos del core del sistema
 */

#include "config_manager.h"
#include "config_commands.h"
#include <WiFi.h>

/*
 * MANEJADORES DE COMANDOS DE CONFIGURACIÓN
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

void ConfigManager::handleConfigRegion(String value) {
    value.trim();
    
    if (value == "US") {
        config.region = REGION_US;
        Serial.println("[OK] Región configurada: US (Estados Unidos/México)");
        Serial.println("[INFO] Frecuencia: " + String(FREQ_US_MHZ) + " MHz");
    }
    else if (value == "EU") {
        config.region = REGION_EU;
        Serial.println("[OK] Región configurada: EU (Europa)");
        Serial.println("[INFO] Frecuencia: " + String(FREQ_EU_MHZ) + " MHz");
    }
    else if (value == "CH") {
        config.region = REGION_CH;
        Serial.println("[OK] Región configurada: CH (China)");
        Serial.println("[INFO] Frecuencia: " + String(FREQ_CH_MHZ) + " MHz");
    }
    else if (value == "AS") {
        config.region = REGION_AS;
        Serial.println("[OK] Región configurada: AS (Asia)");
        Serial.println("[INFO] Frecuencia: " + String(FREQ_AS_MHZ) + " MHz");
    }
    else if (value == "JP") {
        config.region = REGION_JP;
        Serial.println("[OK] Región configurada: JP (Japón)");
        Serial.println("[INFO] Frecuencia: " + String(FREQ_JP_MHZ) + " MHz");
    }
    else {
        Serial.println("[ERROR] Región inválida. Use: US, EU, CH, AS, o JP");
        Serial.println("[INFO] US: 915 MHz (Estados Unidos/México)");
        Serial.println("[INFO] EU: 868 MHz (Europa)");
        Serial.println("[INFO] CH: 470 MHz (China)");
        Serial.println("[INFO] AS: 433 MHz (Asia)");
        Serial.println("[INFO] JP: 920 MHz (Japón)");
        return;
    }
    
    Serial.println("[WARNING] Reinicie el dispositivo después de CONFIG_SAVE para aplicar nueva frecuencia");
}

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
    while (millis() - startTime < CONFIRMATION_TIMEOUT) {
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
    Serial.println("CONFIG_REGION <US|EU|CH|AS|JP>           - Región LoRa (frecuencia)");
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
    Serial.println("=== REGIONES LORA ===");
    Serial.println("US: 915 MHz (Estados Unidos/México)");
    Serial.println("EU: 868 MHz (Europa)");
    Serial.println("CH: 470 MHz (China)");
    Serial.println("AS: 433 MHz (Asia)");
    Serial.println("JP: 920 MHz (Japón)");
    Serial.println("");
    Serial.println("=== MODOS DE DATOS ===");
    Serial.println("SIMPLE: Solo packet [deviceID, lat, lon, battery, timestamp]");
    Serial.println("ADMIN:  Información completa de mesh y estadísticas");
    Serial.println("============================");
}

/*
 * MÉTODOS UTILITARIOS DE STRINGS
 */

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

String ConfigManager::getRegionString(LoRaRegion region) {
    switch (region) {
        case REGION_US: return "US";
        case REGION_EU: return "EU";
        case REGION_CH: return "CH";
        case REGION_AS: return "AS";
        case REGION_JP: return "JP";
        default: return "DESCONOCIDO";
    }
}