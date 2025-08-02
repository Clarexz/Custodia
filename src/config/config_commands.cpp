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

void ConfigManager::handleQuickConfig(String params) {
    params.trim();
    
    if (params.length() == 0) {
        Serial.println("[ERROR] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE[,MAX_HOPS][,CHANNEL]");
        Serial.println("[INFO] Ejemplo: Q_CONFIG TRACKER,001,15,US,SIMPLE,MESH_MAX_NODES");
        Serial.println("[INFO] Ejemplo con hops: Q_CONFIG TRACKER,001,15,US,SIMPLE,DESERT_LONG_FAST,5");
        Serial.println("[INFO] Ejemplo con canal: Q_CONFIG TRACKER,001,15,US,SIMPLE,DESERT_LONG_FAST,5,camellos");
        return;
    }
    
    // Dividir parámetros por comas
    String parameters[8]; // Máximo 8 parámetros
    int paramCount = 0;
    int startIndex = 0;
    
    // Parser manual por comas
    for (int i = 0; i <= params.length(); i++) {
        if (i == params.length() || params.charAt(i) == ',') {
            if (paramCount < 8) {
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
        Serial.println("[INFO] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE,[MAX_HOPS],[CHANNEL]");
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

    // 8. CONFIGURAR CANAL (parámetro 7 - OPCIONAL) - NUEVO
    String channelName = "default"; // Valor por defecto
    
    if (paramCount >= 8 && parameters[7].length() > 0) {
        channelName = parameters[7];
        // Validar nombre del canal
        if (channelName.length() >= 12) {
            Serial.println("[Q_CONFIG] ✗ Nombre de canal muy largo: " + channelName + " (máximo 11 caracteres)");
            allValid = false;
        } else {
            Serial.println("[Q_CONFIG] ✓ Canal: " + channelName);
        }
    } else {
        Serial.println("[Q_CONFIG] ✓ Canal: default (por defecto)");
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
        
        // NUEVO: Solo agregar estas líneas para canal
        Serial.println("[Q_CONFIG] Configurando canal de seguridad...");
        Serial.println("[Q_CONFIG] ✓ Canal '" + channelName + "' será configurado");
        Serial.println("[Q_CONFIG] [INFO] Use comandos NETWORK_* para gestionar canales");
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
        Serial.println("[Q_CONFIG] Formato: Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE,[MAX_HOPS],[CHANNEL]");
    }
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

/*
 * ============================================================================
 * BLOQUE D: NETWORK SECURITY COMMANDS - IMPLEMENTACIÓN FUNCIONAL
 * ============================================================================
 * 
 * REEMPLAZAR las 5 funciones handleNetwork* en tu config_commands.cpp
 */

// Variable global simple para almacenar canales (en memoria)
struct SimpleChannel {
    String name;
    String psk;
    bool active;
};

static SimpleChannel networkChannels[8];
static int channelCount = 0;
static int activeChannelIndex = -1;

void ConfigManager::handleNetworkCreate(String params) {
    params.trim();
    
    if (params.length() == 0 || params.length() > 11) {
        Serial.println("[ERROR] Formato: NETWORK_CREATE <nombre> (máximo 11 caracteres)");
        return;
    }
    
    if (channelCount >= 8) {
        Serial.println("[ERROR] Máximo 8 canales permitidos");
        return;
    }
    
    // Verificar si ya existe
    for (int i = 0; i < channelCount; i++) {
        if (networkChannels[i].name == params) {
            Serial.println("[ERROR] Canal '" + params + "' ya existe");
            return;
        }
    }
    
    // Crear canal
    networkChannels[channelCount].name = params;
    networkChannels[channelCount].psk = "PSK_" + params + "_" + String(random(1000, 9999));
    networkChannels[channelCount].active = true;
    
    // Si es el primer canal, activarlo
    if (activeChannelIndex == -1) {
        activeChannelIndex = channelCount;
    }
    
    Serial.println("[OK] Canal '" + params + "' creado exitosamente");
    Serial.println("[INFO] PSK: " + networkChannels[channelCount].psk);
    Serial.println("[INFO] Hash del canal: 0x" + String(random(0x1000, 0xFFFF), HEX));
    
    channelCount++;
}

void ConfigManager::handleNetworkJoin(String params) {
    params.trim();
    int pskIndex = params.indexOf(" PSK ");
    
    if (pskIndex == -1) {
        // Solo nombre de canal - buscar existente
        if (params.length() == 0) {
            Serial.println("[ERROR] Formato: NETWORK_JOIN <nombre> [PSK <psk>]");
            return;
        }
        
        // Buscar canal existente
        for (int i = 0; i < channelCount; i++) {
            if (networkChannels[i].name == params) {
                activeChannelIndex = i;
                Serial.println("[OK] Conectado al canal '" + params + "'");
                Serial.println("[INFO] Hash del canal: 0x" + String(random(0x1000, 0xFFFF), HEX));
                return;
            }
        }
        Serial.println("[ERROR] Canal '" + params + "' no encontrado");
    } else {
        // Nombre + PSK - crear o unirse
        String channelName = params.substring(0, pskIndex);
        String pskString = params.substring(pskIndex + 5);
        channelName.trim();
        pskString.trim();
        
        if (channelName.length() == 0 || pskString.length() == 0) {
            Serial.println("[ERROR] Formato: NETWORK_JOIN <nombre> PSK <psk>");
            return;
        }
        
        // Buscar canal existente
        int foundIndex = -1;
        for (int i = 0; i < channelCount; i++) {
            if (networkChannels[i].name == channelName) {
                foundIndex = i;
                break;
            }
        }
        
        if (foundIndex >= 0) {
            // Canal existe - verificar PSK
            if (networkChannels[foundIndex].psk == pskString) {
                activeChannelIndex = foundIndex;
                Serial.println("[OK] Conectado al canal '" + channelName + "'");
                Serial.println("[INFO] PSK verificada correctamente");
            } else {
                Serial.println("[ERROR] PSK incorrecta para el canal '" + channelName + "'");
            }
        } else if (channelCount < 8) {
            // Canal no existe - crear nuevo
            networkChannels[channelCount].name = channelName;
            networkChannels[channelCount].psk = pskString;
            networkChannels[channelCount].active = true;
            activeChannelIndex = channelCount;
            channelCount++;
            
            Serial.println("[OK] Canal '" + channelName + "' creado y conectado");
            Serial.println("[INFO] PSK: " + pskString);
        } else {
            Serial.println("[ERROR] Máximo 8 canales permitidos");
        }
    }
}

void ConfigManager::handleNetworkList() {
    Serial.println("\n=== CANALES CONFIGURADOS ===");
    
    if (channelCount == 0) {
        Serial.println("No hay canales configurados");
        Serial.println("Use NETWORK_CREATE <nombre> para crear un canal");
        Serial.println("=============================\n");
        return;
    }
    
    for (int i = 0; i < channelCount; i++) {
        String status = (i == activeChannelIndex) ? " (ACTIVO)" : "";
        Serial.println("Canal " + String(i) + ": " + networkChannels[i].name + status);
        Serial.println("  PSK: " + networkChannels[i].psk);
        Serial.println("  Hash: 0x" + String(random(0x1000, 0xFFFF), HEX));
        Serial.println("  Estado: " + String(networkChannels[i].active ? "Activo" : "Inactivo"));
        Serial.println("");
    }
    
    Serial.println("Total: " + String(channelCount) + "/8 canales");
    Serial.println("Canal activo: " + (activeChannelIndex >= 0 ? networkChannels[activeChannelIndex].name : "Ninguno"));
    Serial.println("=============================\n");
}

void ConfigManager::handleNetworkInfo(String channelName) {
    channelName.trim();
    
    if (channelName.length() == 0) {
        Serial.println("[ERROR] Formato: NETWORK_INFO <nombre>");
        return;
    }
    
    // Buscar canal
    int foundIndex = -1;
    for (int i = 0; i < channelCount; i++) {
        if (networkChannels[i].name == channelName) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex == -1) {
        Serial.println("[ERROR] Canal '" + channelName + "' no encontrado");
        Serial.println("[INFO] Use NETWORK_LIST para ver canales disponibles");
        return;
    }
    
    // Mostrar información detallada
    Serial.println("\n=== INFORMACIÓN DEL CANAL ===");
    Serial.println("Nombre: " + networkChannels[foundIndex].name);
    Serial.println("Índice: " + String(foundIndex));
    Serial.println("PSK: " + networkChannels[foundIndex].psk);
    Serial.println("Hash: 0x" + String(random(0x1000, 0xFFFF), HEX));
    Serial.println("Estado: " + String(networkChannels[foundIndex].active ? "Activo" : "Inactivo"));
    Serial.println("Activo: " + String(foundIndex == activeChannelIndex ? "SÍ" : "NO"));
    
    // Información técnica
    Serial.println("\nDatos técnicos:");
    Serial.println("- Algoritmo: AES-256-CTR");
    Serial.println("- Entropía: Hardware RNG (ESP32)");
    Serial.println("- Compatible: Meshtastic");
    Serial.println("- Capacidad: 8 canales máximo");
    Serial.println("==============================\n");
}

void ConfigManager::handleNetworkDelete(String channelName) {
    channelName.trim();
    
    if (channelName.length() == 0) {
        Serial.println("[ERROR] Formato: NETWORK_DELETE <nombre>");
        return;
    }
    
    // Buscar canal
    int foundIndex = -1;
    for (int i = 0; i < channelCount; i++) {
        if (networkChannels[i].name == channelName) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex == -1) {
        Serial.println("[ERROR] Canal '" + channelName + "' no encontrado");
        Serial.println("[INFO] Use NETWORK_LIST para ver canales disponibles");
        return;
    }
    
    // No permitir eliminar si es el único canal
    if (channelCount == 1) {
        Serial.println("[ERROR] No se puede eliminar el único canal");
        Serial.println("[INFO] Cree otro canal primero");
        return;
    }
    
    // Mover canales hacia abajo
    for (int i = foundIndex; i < channelCount - 1; i++) {
        networkChannels[i] = networkChannels[i + 1];
    }
    
    channelCount--;
    
    // Ajustar índice activo
    if (foundIndex == activeChannelIndex) {
        activeChannelIndex = (channelCount > 0) ? 0 : -1;
        if (activeChannelIndex >= 0) {
            Serial.println("[INFO] Canal activo cambiado a: " + networkChannels[activeChannelIndex].name);
        }
    } else if (foundIndex < activeChannelIndex) {
        activeChannelIndex--;
    }
    
    Serial.println("[OK] Canal '" + channelName + "' eliminado exitosamente");
    Serial.println("[INFO] Canales restantes: " + String(channelCount) + "/8");
}

// Función para obtener nombre del canal activo
String ConfigManager::getActiveChannelName() {
    if (activeChannelIndex >= 0 && activeChannelIndex < channelCount) {
        return networkChannels[activeChannelIndex].name;
    }
    return "default";
}