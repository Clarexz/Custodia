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
    Serial.println("=== COMANDOS DE NETWORKS ===");
    Serial.println("NETWORK_CREATE <nombre> [password]  - Crear nueva network");
    Serial.println("NETWORK_JOIN <nombre> <password>    - Unirse a network");
    Serial.println("NETWORK_LIST                        - Listar networks guardadas");
    Serial.println("NETWORK_INFO [nombre]               - Info detallada de network");
    Serial.println("NETWORK_STATUS                      - Estado del sistema networks");
    Serial.println("NETWORK_DELETE <nombre>             - Eliminar network");
    Serial.println("NETWORK_DELETE_CONFIRM <nombre>     - Confirmar eliminación");
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

void ConfigManager::handleQuickConfig(String params) {
    params.trim();
    
    if (params.length() == 0) {
        Serial.println("[ERROR] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE[,MAX_HOPS]");
        Serial.println("[INFO] Ejemplo: Q_CONFIG TRACKER,001,15,US,SIMPLE,MESH_MAX_NODES");
        Serial.println("[INFO] Ejemplo con hops: Q_CONFIG TRACKER,001,15,US,SIMPLE,DESERT_LONG_FAST,5");
        return;
    }
    
    // Dividir parámetros por comas
    String parameters[7]; // Máximo 7 parámetros
    int paramCount = 0;
    int startIndex = 0;
    
    // Parser manual por comas
    for (int i = 0; i <= params.length(); i++) {
        if (i == params.length() || params.charAt(i) == ',') {
            if (paramCount < 7) {
                parameters[paramCount] = params.substring(startIndex, i);
                parameters[paramCount].trim();
                paramCount++;
            }
            startIndex = i + 1;
        }
    }
    
    // Validar número mínimo de parámetros
    if (paramCount < 6) {
        Serial.println("[ERROR] Faltan parámetros obligatorios");
        Serial.println("[INFO] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE[,MAX_HOPS]");
        Serial.println("[INFO] Parámetros recibidos: " + String(paramCount) + "/6 mínimos");
        return;
    }
    
    Serial.println("[Q_CONFIG] Iniciando configuración rápida...");
    
    // Variables para validar configuración completa
    bool allValid = true;
    
    // 1. CONFIGURAR ROL (parámetro 0)
    String role = parameters[0];
    role.toUpperCase();
    
    if (role == "TRACKER") {
        config.role = ROLE_TRACKER;
        Serial.println("[Q_CONFIG] ✓ Rol: TRACKER");
    }
    else if (role == "REPEATER") {
        config.role = ROLE_REPEATER;
        Serial.println("[Q_CONFIG] ✓ Rol: REPEATER");
    }
    else if (role == "RECEIVER") {
        config.role = ROLE_RECEIVER;
        Serial.println("[Q_CONFIG] ✓ Rol: RECEIVER");
    }
    else {
        Serial.println("[Q_CONFIG] ✗ Rol inválido: " + role + " (use: TRACKER, REPEATER, RECEIVER)");
        allValid = false;
    }
    
    // 2. CONFIGURAR DEVICE ID (parámetro 1)
    int deviceId = parameters[1].toInt();
    
    if (deviceId >= 1 && deviceId <= 999) {
        config.deviceID = deviceId;
        Serial.println("[Q_CONFIG] ✓ Device ID: " + String(deviceId));
    } else {
        Serial.println("[Q_CONFIG] ✗ Device ID inválido: " + parameters[1] + " (use: 1-999)");
        allValid = false;
    }
    
    // 3. CONFIGURAR GPS INTERVAL (parámetro 2)
    int gpsInterval = parameters[2].toInt();
    
    if (gpsInterval >= 5 && gpsInterval <= 3600) {
        config.gpsInterval = gpsInterval;
        Serial.println("[Q_CONFIG] ✓ GPS Interval: " + String(gpsInterval) + " segundos");
    } else {
        Serial.println("[Q_CONFIG] ✗ GPS Interval inválido: " + parameters[2] + " (use: 5-3600)");
        allValid = false;
    }
    
    // 4. CONFIGURAR REGIÓN (parámetro 3)
    String region = parameters[3];
    region.toUpperCase();
    
    if (region == "US") {
        config.region = REGION_US;
        Serial.println("[Q_CONFIG] ✓ Región: US (915 MHz)");
    }
    else if (region == "EU") {
        config.region = REGION_EU;
        Serial.println("[Q_CONFIG] ✓ Región: EU (868 MHz)");
    }
    else if (region == "CH") {
        config.region = REGION_CH;
        Serial.println("[Q_CONFIG] ✓ Región: CH (470 MHz)");
    }
    else if (region == "AS") {
        config.region = REGION_AS;
        Serial.println("[Q_CONFIG] ✓ Región: AS (433 MHz)");
    }
    else if (region == "JP") {
        config.region = REGION_JP;
        Serial.println("[Q_CONFIG] ✓ Región: JP (920 MHz)");
    }
    else {
        Serial.println("[Q_CONFIG] ✗ Región inválida: " + region + " (use: US, EU, CH, AS, JP)");
        allValid = false;
    }
    
    // 5. CONFIGURAR DATA MODE (parámetro 4)
    String dataMode = parameters[4];
    dataMode.toUpperCase();
    
    if (dataMode == "SIMPLE") {
        config.dataMode = DATA_MODE_SIMPLE;
        Serial.println("[Q_CONFIG] ✓ Modo datos: SIMPLE");
    }
    else if (dataMode == "ADMIN") {
        config.dataMode = DATA_MODE_ADMIN;
        Serial.println("[Q_CONFIG] ✓ Modo datos: ADMIN");
    }
    else {
        Serial.println("[Q_CONFIG] ✗ Modo datos inválido: " + dataMode + " (use: SIMPLE, ADMIN)");
        allValid = false;
    }
    
    // 6. CONFIGURAR RADIO PROFILE (parámetro 5 - OBLIGATORIO)
    String radioProfile = parameters[5];
    radioProfile.toUpperCase();
    
    if (radioProfile == "DESERT_LONG_FAST" || radioProfile == "DESERT") {
        config.radioProfile = PROFILE_DESERT_LONG_FAST;
        Serial.println("[Q_CONFIG] ✓ Radio Profile: DESERT_LONG_FAST");
    }
    else if (radioProfile == "MOUNTAIN_STABLE" || radioProfile == "MOUNTAIN") {
        config.radioProfile = PROFILE_MOUNTAIN_STABLE;
        Serial.println("[Q_CONFIG] ✓ Radio Profile: MOUNTAIN_STABLE");
    }
    else if (radioProfile == "URBAN_DENSE" || radioProfile == "URBAN") {
        config.radioProfile = PROFILE_URBAN_DENSE;
        Serial.println("[Q_CONFIG] ✓ Radio Profile: URBAN_DENSE");
    }
    else if (radioProfile == "MESH_MAX_NODES" || radioProfile == "MESH") {
        config.radioProfile = PROFILE_MESH_MAX_NODES;
        Serial.println("[Q_CONFIG] ✓ Radio Profile: MESH_MAX_NODES");
    }
    else if (radioProfile == "CUSTOM_ADVANCED" || radioProfile == "CUSTOM") {
        config.radioProfile = PROFILE_CUSTOM_ADVANCED;
        Serial.println("[Q_CONFIG] ✓ Radio Profile: CUSTOM_ADVANCED");
    }
    else {
        Serial.println("[Q_CONFIG] ✗ Radio Profile inválido: " + radioProfile);
        Serial.println("[Q_CONFIG]   Opciones: DESERT_LONG_FAST, MOUNTAIN_STABLE, URBAN_DENSE, MESH_MAX_NODES, CUSTOM_ADVANCED");
        allValid = false;
    }
    
    // 7. CONFIGURAR MAX HOPS (parámetro 6 - OPCIONAL)
    if (paramCount >= 7) {
        int maxHops = parameters[6].toInt();
        
        if (maxHops >= 1 && maxHops <= 10) {
            config.maxHops = maxHops;
            Serial.println("[Q_CONFIG] ✓ Max hops: " + String(maxHops));
        } else {
            Serial.println("[Q_CONFIG] ✗ Max hops inválido: " + parameters[6] + " (use: 1-10)");
            allValid = false;
        }
    } else {
        // Usar valor por defecto
        config.maxHops = 3;
        Serial.println("[Q_CONFIG] ✓ Max hops: 3 (por defecto)");
    }
    
    // VALIDACIÓN FINAL Y GUARDADO
    if (allValid) {
        // Marcar configuración como válida
        config.configValid = true;
        
        // Aplicar perfil LoRa inmediatamente
        if (radioProfileManager.applyProfile(config.radioProfile)) {
            Serial.println("[Q_CONFIG] ✓ Perfil LoRa aplicado al hardware");
        } else {
            Serial.println("[Q_CONFIG] ⚠ Warning: Error aplicando perfil LoRa");
        }
        
        // Guardar automáticamente
        saveConfig();
        
        Serial.println("[Q_CONFIG] ========================================");
        Serial.println("[Q_CONFIG] CONFIGURACIÓN COMPLETADA EXITOSAMENTE");
        Serial.println("[Q_CONFIG] ========================================");
        
        // Mostrar resumen
        printConfig();
        
        // Iniciar automáticamente
        Serial.println("[Q_CONFIG] Iniciando modo operativo automáticamente...");
        currentState = STATE_RUNNING;
        Serial.println("[Q_CONFIG] Sistema listo y operando");
        
    } else {
        Serial.println("[Q_CONFIG] ========================================");
        Serial.println("[Q_CONFIG] CONFIGURACIÓN FALLÓ");
        Serial.println("[Q_CONFIG] ========================================");
        Serial.println("[Q_CONFIG] Corrija los errores e intente nuevamente");
        Serial.println("[Q_CONFIG] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE[,MAX_HOPS]");
    }
}

/*
 * ===== COMANDOS DE NETWORKS =====
 */

void ConfigManager::handleNetworkList() {
    Serial.println("========================================");
    Serial.println("           NETWORKS GUARDADAS");
    Serial.println("========================================");
    
    if (networkCount == 0) {
        Serial.println("[INFO] No hay networks guardadas.");
        Serial.println("[INFO] Use 'NETWORK_CREATE <nombre> [password]' para crear una.");
        Serial.println("========================================");
        return;
    }
    
    // Mostrar cada network guardada
    for (int i = 0; i < networkCount; i++) {
        SimpleNetwork* net = &networks[i];
        
        // Indicador de network activa
        String activeIndicator = net->active ? " [ACTIVA]" : "";
        
        Serial.println("Network " + String(i + 1) + ":" + activeIndicator);
        Serial.println("  Nombre:   " + net->name);
        Serial.println("  Password: " + net->password);
        Serial.println("  Hash:     " + String(net->hash, HEX));
        
        if (i < networkCount - 1) {
            Serial.println("  ----");
        }
    }
    
    Serial.println("========================================");
    Serial.println("Total: " + String(networkCount) + "/" + String(MAX_NETWORKS) + " networks");
    
    // Mostrar network activa
    if (hasActiveNetwork()) {
        SimpleNetwork* active = getActiveNetwork();
        Serial.println("Network activa: " + active->name);
    } else {
        Serial.println("Ninguna network activa");
    }
    
    Serial.println("========================================");
}

void ConfigManager::handleNetworkCreate(String params) {
    params.trim();
    
    // Verificar que no excedamos el límite de networks
    if (networkCount >= MAX_NETWORKS) {
        Serial.println("[ERROR] Máximo de " + String(MAX_NETWORKS) + " networks alcanzado.");
        Serial.println("[INFO] Use 'NETWORK_LIST' para ver networks existentes.");
        return;
    }
    
    // Parsear parámetros: <nombre> [password]
    int spaceIndex = params.indexOf(' ');
    String name = "";
    String password = "";
    
    if (spaceIndex > 0) {
        // Hay password especificada
        name = params.substring(0, spaceIndex);
        password = params.substring(spaceIndex + 1);
        password.trim();
    } else {
        // Solo nombre, generar password automática
        name = params;
        password = generateRandomPassword();
        Serial.println("[INFO] Password auto-generada: " + password);
    }
    
    name.trim();
    
    // Validar nombre
    if (!isValidNetworkName(name)) {
        Serial.println("[ERROR] Nombre inválido. Use 3-20 caracteres alfanuméricos, guiones o underscore.");
        return;
    }
    
    // Validar password
    if (!isValidPassword(password)) {
        Serial.println("[ERROR] Password inválida. Use 8-32 caracteres alfanuméricos.");
        return;
    }
    
    // Verificar que no exista ya una network con este nombre
    if (findNetworkByName(name) >= 0) {
        Serial.println("[ERROR] Ya existe una network con el nombre '" + name + "'.");
        return;
    }
    
    // Crear nueva network
    networks[networkCount] = SimpleNetwork(name, password);
    
    // Si es la primera network, activarla automáticamente
    if (networkCount == 0) {
        networks[networkCount].active = true;
        activeNetworkIndex = 0;
        Serial.println("[INFO] Primera network creada - activada automáticamente.");
    }
    
    networkCount++;
    
    // Mostrar confirmación
    Serial.println("[OK] Network '" + name + "' creada exitosamente.");
    Serial.println("[INFO] Nombre: " + networks[networkCount-1].name);
    Serial.println("[INFO] Password: " + networks[networkCount-1].password);
    Serial.println("[INFO] Hash: " + String(networks[networkCount-1].hash, HEX));
    
    if (networks[networkCount-1].active) {
        Serial.println("[INFO] Network activa: " + networks[networkCount-1].name);
    }
    
    Serial.println("[INFO] Use 'CONFIG_SAVE' para guardar la configuración.");
}

void ConfigManager::handleNetworkJoin(String params) {
    params.trim();
    
    // Verificar que hay parámetros
    if (params.length() == 0) {
        Serial.println("[ERROR] Formato: NETWORK_JOIN <nombre> <password>");
        Serial.println("[INFO] Use 'NETWORK_LIST' para ver networks disponibles.");
        return;
    }
    
    // Parsear parámetros: <nombre> <password>
    int spaceIndex = params.indexOf(' ');
    
    if (spaceIndex <= 0) {
        Serial.println("[ERROR] Formato: NETWORK_JOIN <nombre> <password>");
        Serial.println("[INFO] Debe especificar tanto nombre como password.");
        return;
    }
    
    String name = params.substring(0, spaceIndex);
    String password = params.substring(spaceIndex + 1);
    
    name.trim();
    password.trim();
    
    // Validar parámetros
    if (!isValidNetworkName(name)) {
        Serial.println("[ERROR] Nombre inválido.");
        return;
    }
    
    if (!isValidPassword(password)) {
        Serial.println("[ERROR] Password inválida.");
        return;
    }
    
    // Buscar network existente
    int networkIndex = findNetworkByName(name);
    
    if (networkIndex >= 0) {
        String upperPassword = password;
        upperPassword.toUpperCase();
        
        // Network existe - verificar password
        if (networks[networkIndex].password == upperPassword) {
            // Password correcta - cambiar network activa
            
            // Desactivar network anterior
            if (activeNetworkIndex >= 0) {
                networks[activeNetworkIndex].active = false;
            }
            
            // Activar nueva network
            networks[networkIndex].active = true;
            activeNetworkIndex = networkIndex;
            
            Serial.println("[OK] Conectado a network '" + networks[networkIndex].name + "'.");
            Serial.println("[INFO] Hash activo: " + String(networks[networkIndex].hash, HEX));
            Serial.println("[INFO] Use 'CONFIG_SAVE' para guardar la configuración.");
        } else {
            Serial.println("[ERROR] Password incorrecta para network '" + name + "'.");
        }
    } else {
        // Network no existe - crear nueva
        if (networkCount >= MAX_NETWORKS) {
            Serial.println("[ERROR] Máximo de " + String(MAX_NETWORKS) + " networks alcanzado.");
            Serial.println("[INFO] No se puede crear nueva network '" + name + "'.");
            return;
        }
        
        // Crear nueva network
        networks[networkCount] = SimpleNetwork(name, password);
        
        // Desactivar network anterior
        if (activeNetworkIndex >= 0) {
            networks[activeNetworkIndex].active = false;
        }
        
        // Activar nueva network
        networks[networkCount].active = true;
        activeNetworkIndex = networkCount;
        networkCount++;
        
        Serial.println("[INFO] Network '" + name + "' no existía - creada y activada.");
        Serial.println("[INFO] Nombre: " + networks[activeNetworkIndex].name);
        Serial.println("[INFO] Password: " + networks[activeNetworkIndex].password);
        Serial.println("[INFO] Hash: " + String(networks[activeNetworkIndex].hash, HEX));
        Serial.println("[INFO] Use 'CONFIG_SAVE' para guardar la configuración.");
    }
}

void ConfigManager::handleNetworkInfo(String params) {
    params.trim();
    
    // Si no hay parámetros, mostrar info de la network activa
    if (params.length() == 0) {
        if (!hasActiveNetwork()) {
            Serial.println("[ERROR] No hay network activa.");
            Serial.println("[INFO] Use 'NETWORK_LIST' para ver networks disponibles.");
            return;
        }
        
        SimpleNetwork* active = getActiveNetwork();
        Serial.println("========================================");
        Serial.println("      INFO NETWORK ACTIVA");
        Serial.println("========================================");
        Serial.println("Nombre:       " + active->name);
        Serial.println("Password:     " + active->password);
        Serial.println("Hash:         " + String(active->hash, HEX));
        Serial.println("Estado:       ACTIVA");
        Serial.println("Longitud pwd: " + String(active->password.length()) + " caracteres");
        Serial.println("Segura:       " + String(isPasswordSecure(active->password) ? "Sí" : "No"));
        Serial.println("========================================");
        return;
    }
    
    // Buscar network específica por nombre
    String networkName = params;
    networkName.trim();
    networkName.toUpperCase();
    
    int networkIndex = findNetworkByName(networkName);
    if (networkIndex < 0) {
        Serial.println("[ERROR] Network '" + params + "' no encontrada.");
        Serial.println("[INFO] Use 'NETWORK_LIST' para ver networks disponibles.");
        return;
    }
    
    SimpleNetwork* net = &networks[networkIndex];
    
    // Mostrar información detallada de la network
    Serial.println("========================================");
    Serial.println("      INFO NETWORK: " + net->name);
    Serial.println("========================================");
    Serial.println("Nombre:       " + net->name);
    Serial.println("Password:     " + net->password);
    Serial.println("Hash:         " + String(net->hash, HEX));
    Serial.println("Estado:       " + String(net->active ? "ACTIVA" : "Inactiva"));
    Serial.println("Longitud pwd: " + String(net->password.length()) + " caracteres");
    Serial.println("Segura:       " + String(isPasswordSecure(net->password) ? "Sí" : "No"));
    Serial.println("Índice:       " + String(networkIndex));
    
    // Mostrar validaciones
    String errorMsg;
    bool nameValid = (validateNetworkNameAdvanced(net->name, errorMsg).length() > 0);
    bool passValid = (validatePasswordAdvanced(net->password, net->name, errorMsg).length() > 0);
    
    Serial.println("Nombre válido: " + String(nameValid ? "Sí" : "No"));
    Serial.println("Password válida: " + String(passValid ? "Sí" : "No"));
    
    Serial.println("========================================");
    Serial.println("Uso memoria:  ~" + String(net->name.length() + net->password.length() + 35) + " bytes");
    Serial.println("========================================");
}

void ConfigManager::handleNetworkDelete(String params) {
    params.trim();
    
    // Verificar que se especificó un nombre
    if (params.length() == 0) {
        Serial.println("[ERROR] Formato: NETWORK_DELETE <nombre>");
        Serial.println("[INFO] Use 'NETWORK_LIST' para ver networks disponibles.");
        return;
    }
    
    String networkName = params;
    networkName.trim();
    String originalName = networkName;
    networkName.toUpperCase();
    
    // Verificar si se puede eliminar (validaciones de seguridad)
    String errorMsg;
    if (!canDeleteNetwork(networkName, errorMsg)) {
        Serial.println("[ERROR] " + errorMsg);
        return;
    }
    
    // Buscar la network a eliminar
    int networkIndex = findNetworkByName(networkName);
    if (networkIndex < 0) {
        Serial.println("[ERROR] Network '" + originalName + "' no encontrada.");
        return;
    }
    
    SimpleNetwork* networkToDelete = &networks[networkIndex];
    bool isDeletingActive = networkToDelete->active;
    
    // Mostrar información de la network a eliminar
    Serial.println("========================================");
    Serial.println("      CONFIRMAR ELIMINACIÓN");
    Serial.println("========================================");
    Serial.println("Network a eliminar: " + networkToDelete->name);
    Serial.println("Password:           " + networkToDelete->password);
    Serial.println("Hash:               " + String(networkToDelete->hash, HEX));
    Serial.println("Estado:             " + String(isDeletingActive ? "ACTIVA" : "Inactiva"));
    
    if (isDeletingActive) {
        Serial.println("");
        Serial.println("[WARNING] Esta es la network ACTIVA!");
        Serial.println("[INFO] Se activará automáticamente otra network.");
    }
    
    Serial.println("========================================");
    Serial.println("¿Está seguro de eliminar esta network?");
    Serial.println("Esta acción NO se puede deshacer.");
    Serial.println("");
    Serial.println("Escriba 'YES' para confirmar o cualquier");
    Serial.println("otra cosa para cancelar:");
    
    Serial.println("");
    Serial.println("[INFO] Comando preparado. La eliminación se ejecutará");
    Serial.println("[INFO] cuando escriba: NETWORK_DELETE_CONFIRM " + networkName);
    Serial.println("[INFO] o cancele con cualquier otro comando.");
    
    return;
}

void ConfigManager::handleNetworkDeleteConfirm(String params) {
    params.trim();
    
    if (params.length() == 0) {
        Serial.println("[ERROR] Formato: NETWORK_DELETE_CONFIRM <nombre>");
        return;
    }
    
    String networkName = params;
    networkName.toUpperCase();
    
    // Verificar nuevamente que se puede eliminar
    String errorMsg;
    if (!canDeleteNetwork(networkName, errorMsg)) {
        Serial.println("[ERROR] " + errorMsg);
        return;
    }
    
    // Buscar la network a eliminar
    int networkIndex = findNetworkByName(networkName);
    if (networkIndex < 0) {
        Serial.println("[ERROR] Network '" + params + "' no encontrada.");
        return;
    }
    
    SimpleNetwork* networkToDelete = &networks[networkIndex];
    bool isDeletingActive = networkToDelete->active;
    String deletedName = networkToDelete->name;
    
    // Eliminar la network (mover todas las siguientes una posición hacia atrás)
    for (int i = networkIndex; i < networkCount - 1; i++) {
        networks[i] = networks[i + 1];
    }
    
    // Reducir contador
    networkCount--;
    
    // Si eliminamos la network activa, activar otra
    if (isDeletingActive) {
        if (networkCount > 0) {
            // Activar la primera network disponible
            networks[0].active = true;
            activeNetworkIndex = 0;
            
            // Desactivar todas las demás
            for (int i = 1; i < networkCount; i++) {
                networks[i].active = false;
            }
            
            Serial.println("[INFO] Network '" + networks[0].name + "' activada automáticamente.");
        } else {
            // No quedan networks
            activeNetworkIndex = -1;
            Serial.println("[INFO] No quedan networks. Sistema sin network activa.");
        }
    } else {
        // Ajustar el índice activo si es necesario
        if (activeNetworkIndex > networkIndex) {
            activeNetworkIndex--;
        }
    }
    
    // Confirmar eliminación
    Serial.println("========================================");
    Serial.println("[OK] Network '" + deletedName + "' eliminada exitosamente.");
    Serial.println("Networks restantes: " + String(networkCount) + "/" + String(MAX_NETWORKS));
    
    if (hasActiveNetwork()) {
        SimpleNetwork* active = getActiveNetwork();
        Serial.println("Network activa: " + active->name);
    } else {
        Serial.println("Network activa: NINGUNA");
    }
    
    Serial.println("========================================");
    Serial.println("[INFO] Use 'CONFIG_SAVE' para guardar los cambios.");
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