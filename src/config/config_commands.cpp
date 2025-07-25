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
        Serial.println("[INFO] Modo actual: " + getCurrentDataModeString());
        return;
    }
}

void ConfigManager::handleConfigRegion(String value) {
    value.trim();
    
    if (value == "US") {
        config.region = REGION_US;
        Serial.println("[OK] Región configurada: US (Estados Unidos/México)");
        Serial.println("[INFO] Frecuencia: " + String(getFrequencyMHz()) + " MHz");
    }
    else if (value == "EU") {
        config.region = REGION_EU;
        Serial.println("[OK] Región configurada: EU (Europa)");
        Serial.println("[INFO] Frecuencia: " + String(getFrequencyMHz()) + " MHz");
    }
    else if (value == "CH") {
        config.region = REGION_CH;
        Serial.println("[OK] Región configurada: CH (China)");
        Serial.println("[INFO] Frecuencia: " + String(getFrequencyMHz()) + " MHz");
    }
    else if (value == "AS") {
        config.region = REGION_AS;
        Serial.println("[OK] Región configurada: AS (Asia)");
        Serial.println("[INFO] Frecuencia: " + String(getFrequencyMHz()) + " MHz");
    }
    else if (value == "JP") {
        config.region = REGION_JP;
        Serial.println("[OK] Región configurada: JP (Japón)");
        Serial.println("[INFO] Frecuencia: " + String(getFrequencyMHz()) + " MHz");
    }
    else {
        Serial.println("[ERROR] Región inválida. Use: US, EU, CH, AS, o JP");
    }
}

// ========== NUEVOS MANEJADORES DE RADIO PROFILES ==========

void ConfigManager::handleConfigRadioProfile(String value) {
    value.trim();
    value.toUpperCase();
    
    // Comando principal: CONFIG_RADIO_PROFILE <profile_name>
    if (value == "DESERT_LONG_FAST" || value == "DESERT") {
        config.radioProfile = PROFILE_DESERT_LONG_FAST;
        if (radioProfileManager.applyProfile(PROFILE_DESERT_LONG_FAST)) {
            Serial.println("[OK] Perfil configurado: DESERT_LONG_FAST");
            Serial.println("[INFO] Optimizado para máximo alcance en terreno abierto");
            Serial.println("[INFO] SF11, 250kHz, ~8km alcance, airtime ~2.2s");
        }
    }
    else if (value == "MOUNTAIN_STABLE" || value == "MOUNTAIN") {
        config.radioProfile = PROFILE_MOUNTAIN_STABLE;
        if (radioProfileManager.applyProfile(PROFILE_MOUNTAIN_STABLE)) {
            Serial.println("[OK] Perfil configurado: MOUNTAIN_STABLE");
            Serial.println("[INFO] Optimizado para condiciones adversas y obstáculos");
            Serial.println("[INFO] SF10, 125kHz, ~4km alcance, airtime ~0.9s");
        }
    }
    else if (value == "URBAN_DENSE" || value == "URBAN") {
        config.radioProfile = PROFILE_URBAN_DENSE;
        if (radioProfileManager.applyProfile(PROFILE_URBAN_DENSE)) {
            Serial.println("[OK] Perfil configurado: URBAN_DENSE");
            Serial.println("[INFO] Optimizado para alta velocidad y testing");
            Serial.println("[INFO] SF7, 500kHz, ~800m alcance, airtime ~80ms");
        }
    }
    else if (value == "MESH_MAX_NODES" || value == "MESH") {
        config.radioProfile = PROFILE_MESH_MAX_NODES;
        if (radioProfileManager.applyProfile(PROFILE_MESH_MAX_NODES)) {
            Serial.println("[OK] Perfil configurado: MESH_MAX_NODES");
            Serial.println("[INFO] Balance optimizado para redes grandes (20-30 nodos)");
            Serial.println("[INFO] SF9, 250kHz, ~2.5km alcance, airtime ~320ms");
        }
    }
    else if (value == "CUSTOM_ADVANCED" || value == "CUSTOM") {
        config.radioProfile = PROFILE_CUSTOM_ADVANCED;
        if (radioProfileManager.applyProfile(PROFILE_CUSTOM_ADVANCED)) {
            Serial.println("[OK] Perfil configurado: CUSTOM_ADVANCED");
            Serial.println("[INFO] Configuración manual activa");
            Serial.println("[INFO] Use RADIO_PROFILE_CUSTOM <param> <value> para configurar");
            Serial.println("[INFO] Parámetros: SF, BW, CR, POWER, PREAMBLE");
        }
    }
    else if (value == "LIST") {
        radioProfileManager.printAllProfiles();
    }
    else if (value.startsWith("INFO ")) {
        String profileName = value.substring(5);
        profileName.trim();
        profileName.toUpperCase();
        
        RadioProfile profile;
        bool valid = true;
        
        if (profileName == "DESERT_LONG_FAST" || profileName == "DESERT") {
            profile = PROFILE_DESERT_LONG_FAST;
        }
        else if (profileName == "MOUNTAIN_STABLE" || profileName == "MOUNTAIN") {
            profile = PROFILE_MOUNTAIN_STABLE;
        }
        else if (profileName == "URBAN_DENSE" || profileName == "URBAN") {
            profile = PROFILE_URBAN_DENSE;
        }
        else if (profileName == "MESH_MAX_NODES" || profileName == "MESH") {
            profile = PROFILE_MESH_MAX_NODES;
        }
        else if (profileName == "CUSTOM_ADVANCED" || profileName == "CUSTOM") {
            profile = PROFILE_CUSTOM_ADVANCED;
        }
        else {
            Serial.println("[ERROR] Perfil desconocido: " + profileName);
            valid = false;
        }
        
        if (valid) {
            radioProfileManager.printProfileInfo(profile);
        }
    }
    else if (value == "COMPARE") {
        radioProfileManager.printProfileComparison();
    }
    else {
        Serial.println("[ERROR] Perfil inválido: " + value);
        Serial.println("[INFO] Perfiles disponibles:");
        Serial.println("  DESERT_LONG_FAST   - Máximo alcance campo abierto");
        Serial.println("  MOUNTAIN_STABLE    - Condiciones adversas");
        Serial.println("  URBAN_DENSE        - Alta velocidad urbana");
        Serial.println("  MESH_MAX_NODES     - Balance redes grandes");
        Serial.println("  CUSTOM_ADVANCED    - Configuración manual");
        Serial.println("[INFO] Comandos: LIST, INFO <perfil>, COMPARE");
    }
}

void ConfigManager::handleRadioProfileCustom(String param, String value) {
    if (config.radioProfile != PROFILE_CUSTOM_ADVANCED) {
        Serial.println("[ERROR] Comando solo disponible con perfil CUSTOM_ADVANCED");
        Serial.println("[INFO] Use: CONFIG_RADIO_PROFILE CUSTOM_ADVANCED primero");
        return;
    }
    
    param.trim();
    param.toUpperCase();
    float numValue = value.toFloat();
    
    bool valid = false;
    
    if (param == "SF") {
        if (numValue >= 7 && numValue <= 12) {
            radioProfileManager.setCustomParameter("SF", numValue);
            Serial.println("[OK] Spreading Factor configurado: SF" + String((int)numValue));
            valid = true;
        } else {
            Serial.println("[ERROR] SF debe estar entre 7 y 12");
        }
    }
    else if (param == "BW") {
        if (numValue == 125.0 || numValue == 250.0 || numValue == 500.0) {
            radioProfileManager.setCustomParameter("BW", numValue);
            Serial.println("[OK] Bandwidth configurado: " + String(numValue) + " kHz");
            valid = true;
        } else {
            Serial.println("[ERROR] BW debe ser 125, 250 o 500 kHz");
        }
    }
    else if (param == "CR") {
        if (numValue >= 5 && numValue <= 8) {
            radioProfileManager.setCustomParameter("CR", numValue);
            Serial.println("[OK] Coding Rate configurado: 4/" + String((int)numValue));
            valid = true;
        } else {
            Serial.println("[ERROR] CR debe estar entre 5 y 8 (para 4/5 a 4/8)");
        }
    }
    else if (param == "POWER") {
        if (numValue >= 2 && numValue <= 20) {
            radioProfileManager.setCustomParameter("POWER", numValue);
            Serial.println("[OK] TX Power configurado: " + String((int)numValue) + " dBm");
            valid = true;
        } else {
            Serial.println("[ERROR] POWER debe estar entre 2 y 20 dBm");
        }
    }
    else if (param == "PREAMBLE") {
        if (numValue >= 6 && numValue <= 65535) {
            radioProfileManager.setCustomParameter("PREAMBLE", numValue);
            Serial.println("[OK] Preamble configurado: " + String((int)numValue) + " símbolos");
            valid = true;
        } else {
            Serial.println("[ERROR] PREAMBLE debe estar entre 6 y 65535");
        }
    }
    else {
        Serial.println("[ERROR] Parámetro desconocido: " + param);
        Serial.println("[INFO] Parámetros válidos: SF, BW, CR, POWER, PREAMBLE");
        return;
    }
    
    if (valid) {
        Serial.println("[INFO] Use RADIO_PROFILE_APPLY para aplicar cambios");
    }
}

void ConfigManager::handleRadioProfileApply() {
    if (config.radioProfile == PROFILE_CUSTOM_ADVANCED) {
        if (radioProfileManager.applyProfile(PROFILE_CUSTOM_ADVANCED)) {
            Serial.println("[OK] Configuración custom aplicada al hardware LoRa");
            if (isAdminMode()) {
                radioProfileManager.printProfileInfo(PROFILE_CUSTOM_ADVANCED);
            }
        } else {
            Serial.println("[ERROR] Error al aplicar configuración custom");
        }
    } else {
        Serial.println("[ERROR] Comando solo disponible en modo CUSTOM_ADVANCED");
        Serial.println("[INFO] Perfil actual: " + getRadioProfileName());
    }
}

void ConfigManager::handleRadioProfileStatus() {
    Serial.println("\n=== STATUS RADIO PROFILE ===");
    Serial.println("Perfil actual: " + getRadioProfileName());
    
    if (isAdminMode()) {
        radioProfileManager.printProfileInfo(config.radioProfile);
    } else {
        RadioProfileConfig profileConfig = radioProfileManager.getProfileConfig(config.radioProfile);
        Serial.println("Alcance estimado: ~" + String(profileConfig.approxRange) + " metros");
        Serial.println("Airtime (44 bytes): " + String(profileConfig.airtimeMs) + " ms");
        Serial.println("Rating batería: " + String(profileConfig.batteryRating) + "/10");
        Serial.println("Rating velocidad: " + String(profileConfig.speedRating) + "/10");
    }
    Serial.println("============================");
}

// =========================================================

void ConfigManager::handleModeChange(String value) {
    value.trim();
    
    if (value == "SIMPLE") {
        setDataMode(DATA_MODE_SIMPLE);
        Serial.println("[OK] Cambiado a modo SIMPLE");
        Serial.println("[INFO] Mostrando solo datos básicos de packets");
    }
    else if (value == "ADMIN") {
        setDataMode(DATA_MODE_ADMIN);
        Serial.println("[OK] Cambiado a modo ADMIN");
        Serial.println("[INFO] Mostrando información completa de mesh");
    }
    else {
        Serial.println("[ERROR] Modo inválido. Use: SIMPLE o ADMIN");
        Serial.println("[INFO] Modo actual: " + getCurrentDataModeString());
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
    Serial.println("");
    Serial.println("=== RADIO PROFILES ===");
    Serial.println("CONFIG_RADIO_PROFILE <perfil>            - Configurar perfil LoRa");
    Serial.println("CONFIG_RADIO_PROFILE LIST                - Listar perfiles disponibles");
    Serial.println("CONFIG_RADIO_PROFILE INFO <perfil>       - Información detallada");
    Serial.println("CONFIG_RADIO_PROFILE COMPARE             - Comparar todos los perfiles");
    Serial.println("RADIO_PROFILE_CUSTOM <param> <value>     - Configurar parámetro custom");
    Serial.println("RADIO_PROFILE_APPLY                      - Aplicar configuración custom");
    Serial.println("RADIO_PROFILE_STATUS                     - Mostrar perfil actual");
    Serial.println("");
    Serial.println("=== PERFILES DISPONIBLES ===");
    Serial.println("DESERT_LONG_FAST     - Máximo alcance (8km, 2.2s, batería 3/10)");
    Serial.println("MOUNTAIN_STABLE      - Condiciones adversas (4km, 0.9s, batería 5/10)");
    Serial.println("URBAN_DENSE          - Alta velocidad (800m, 80ms, batería 8/10)");
    Serial.println("MESH_MAX_NODES       - Balance redes grandes (2.5km, 320ms, batería 7/10)");
    Serial.println("CUSTOM_ADVANCED      - Configuración manual experta");
    Serial.println("");
    Serial.println("=== PARÁMETROS CUSTOM ===");
    Serial.println("SF (7-12), BW (125/250/500), CR (5-8), POWER (2-20), PREAMBLE (6-65535)");
    Serial.println("Ejemplo: RADIO_PROFILE_CUSTOM SF 10");
    Serial.println("");
    Serial.println("=== COMANDOS DE GESTIÓN ===");
    Serial.println("CONFIG_SAVE                              - Guardar configuración");
    Serial.println("CONFIG_RESET                             - Resetear configuración");
    Serial.println("INFO                                     - Información del dispositivo");
    Serial.println("STATUS                                   - Estado del sistema");
    Serial.println("START                                    - Iniciar modo operativo");
    Serial.println("HELP                                     - Mostrar esta ayuda");
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
        case ROLE_NONE: return "NONE";
        default: return "UNKNOWN";
    }
}

String ConfigManager::getStateString(SystemState state) {
    switch (state) {
        case STATE_BOOT: return "BOOT";
        case STATE_CONFIG_MODE: return "CONFIG_MODE";
        case STATE_RUNNING: return "RUNNING";
        case STATE_SLEEP: return "SLEEP";
        default: return "UNKNOWN";
    }
}

String ConfigManager::getDataModeString(DataDisplayMode mode) {
    switch (mode) {
        case DATA_MODE_SIMPLE: return "SIMPLE";
        case DATA_MODE_ADMIN: return "ADMIN";
        default: return "UNKNOWN";
    }
}

String ConfigManager::getRegionString(LoRaRegion region) {
    switch (region) {
        case REGION_US: return "US";
        case REGION_EU: return "EU";
        case REGION_CH: return "CH";
        case REGION_AS: return "AS";
        case REGION_JP: return "JP";
        default: return "UNKNOWN";
    }
}