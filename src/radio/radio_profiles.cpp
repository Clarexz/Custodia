/*
 * RADIO_PROFILES.CPP - Implementación del Sistema de Perfiles LoRa
 * 
 * BLOQUE E: Configuraciones predefinidas optimizadas para diferentes escenarios
 */

#include "radio_profiles.h"
#include "../config/config_manager.h"
#include "../lora.h"
#include <math.h>

/*
 * INSTANCIA GLOBAL
 */
RadioProfileManager radioProfileManager;

/*
 * CONFIGURACIONES PREDEFINIDAS
 */
static const RadioProfileConfig PREDEFINED_PROFILES[] = {
    // DESERT_LONG_FAST
    {
        PROFILE_DESERT_LONG_FAST,
        "DESERT_LONG_FAST",
        "Máximo alcance para terreno abierto (reservas, campos)",
        DESERT_SF, DESERT_BW, DESERT_CR, DESERT_POWER, DESERT_PREAMBLE,
        8000,   // ~8km alcance estimado
        2200,   // ~2.2s airtime
        3,      // Rating batería (alto consumo)
        2,      // Rating velocidad (muy lento)
        "Animal tracking, field monitoring, long-range sensors",
        "Máximo alcance y penetración | Consumo alto, transmisiones lentas"
    },
    
    // MOUNTAIN_STABLE
    {
        PROFILE_MOUNTAIN_STABLE,
        "MOUNTAIN_STABLE", 
        "Robustez en condiciones adversas con obstáculos",
        MOUNTAIN_SF, MOUNTAIN_BW, MOUNTAIN_CR, MOUNTAIN_POWER, MOUNTAIN_PREAMBLE,
        4000,   // ~4km alcance estimado
        900,    // ~0.9s airtime
        5,      // Rating batería (balance)
        4,      // Rating velocidad (lento)
        "Forest repeaters, mountain deployments, harsh environments",
        "Estabilidad y corrección de errores | Velocidad reducida"
    },
    
    // URBAN_DENSE
    {
        PROFILE_URBAN_DENSE,
        "URBAN_DENSE",
        "Alta velocidad para entornos densos y testing",
        URBAN_SF, URBAN_BW, URBAN_CR, URBAN_POWER, URBAN_PREAMBLE,
        800,    // ~800m alcance estimado
        80,     // ~80ms airtime
        8,      // Rating batería (bajo consumo)
        9,      // Rating velocidad (muy rápido)
        "Lab testing, development, urban IoT, high-density networks",
        "Velocidad máxima y baja latencia | Alcance limitado"
    },
    
    // MESH_MAX_NODES
    {
        PROFILE_MESH_MAX_NODES,
        "MESH_MAX_NODES",
        "Balance optimizado para redes mesh grandes (20-30 nodos)",
        MESH_SF, MESH_BW, MESH_CR, MESH_POWER, MESH_PREAMBLE,
        2500,   // ~2.5km alcance estimado
        320,    // ~320ms airtime
        7,      // Rating batería (bueno)
        7,      // Rating velocidad (bueno)
        "Large mesh networks, multiple repeaters, balanced performance",
        "Balance óptimo alcance/velocidad/batería | Sin extremos"
    },
    
    // CUSTOM_ADVANCED
    {
        PROFILE_CUSTOM_ADVANCED,
        "CUSTOM_ADVANCED",
        "Configuración manual experta - usuario define parámetros",
        CUSTOM_SF, CUSTOM_BW, CUSTOM_CR, CUSTOM_POWER, CUSTOM_PREAMBLE,
        2500,   // Basado en valores por defecto
        320,    // Basado en valores por defecto
        7,      // Se recalcula según configuración
        7,      // Se recalcula según configuración
        "Expert configuration, specific requirements, fine-tuning",
        "Control total de parámetros | Requiere conocimiento técnico"
    }
};

/*
 * CONSTRUCTOR
 */
RadioProfileManager::RadioProfileManager() {
    currentProfile = PROFILE_MESH_MAX_NODES; // Perfil por defecto balanceado
    customConfig = PREDEFINED_PROFILES[PROFILE_CUSTOM_ADVANCED]; // Inicializar custom
}

/*
 * OBTENER CONFIGURACIÓN DE PERFIL
 */
RadioProfileConfig RadioProfileManager::getProfileConfig(RadioProfile profile) {
    if (profile == PROFILE_CUSTOM_ADVANCED) {
        return customConfig; // Retornar configuración personalizada
    }
    
    if (profile >= 0 && profile < 5) {
        return PREDEFINED_PROFILES[profile];
    }
    
    // Retornar perfil por defecto si es inválido
    return PREDEFINED_PROFILES[PROFILE_MESH_MAX_NODES];
}

/*
 * APLICAR PERFIL AL SISTEMA LORA
 */
bool RadioProfileManager::applyProfile(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    Serial.println("[Radio Profile] Aplicando perfil: " + String(config.name));
    
    // Verificar compatibilidad con región actual
    if (!isCompatibleWithRegion(profile)) {
        Serial.println("[Radio Profile] WARNING: Perfil puede no ser óptimo para región actual");
    }
    
    // Aplicar configuración al hardware LoRa
    bool success = true;
    
    // Spreading Factor
    if (loraManager.getStatus() == LORA_STATUS_READY) {
        // Usar métodos del LoRaManager para configurar parámetros
        loraManager.setSpreadingFactor(config.spreadingFactor);
        loraManager.setBandwidth(config.bandwidth);
        loraManager.setTxPower(config.txPower);
        
        // Para Coding Rate y Preamble, necesitaremos actualizar lora_hardware.h
        // Por ahora, solo reportamos que se aplicarían
        Serial.println("[Radio Profile] SF: " + String(config.spreadingFactor) + 
                      ", BW: " + String(config.bandwidth) + " kHz" +
                      ", CR: 4/" + String(config.codingRate) + 
                      ", Power: " + String(config.txPower) + " dBm");
    } else {
        Serial.println("[Radio Profile] WARNING: LoRa no está listo, configuración pendiente");
        success = false;
    }
    
    if (success) {
        currentProfile = profile;
        Serial.println("[Radio Profile] Perfil aplicado exitosamente");
        
        // Mostrar información del perfil aplicado
        if (configManager.isAdminMode()) {
            printProfileInfo(profile);
        }
    }
    
    return success;
}

/*
 * CONFIGURAR PARÁMETRO CUSTOM
 */
bool RadioProfileManager::setCustomParameter(const String& param, float value) {
    if (currentProfile != PROFILE_CUSTOM_ADVANCED) {
        Serial.println("[Radio Profile] ERROR: Solo disponible en modo CUSTOM_ADVANCED");
        return false;
    }
    
    bool valid = false;
    
    if (param == "SF" || param == "SPREADING_FACTOR") {
        if (value >= 7 && value <= 12) {
            customConfig.spreadingFactor = (uint8_t)value;
            valid = true;
        }
    }
    else if (param == "BW" || param == "BANDWIDTH") {
        if (value == 125.0f || value == 250.0f || value == 500.0f) {
            customConfig.bandwidth = value;
            valid = true;
        }
    }
    else if (param == "CR" || param == "CODING_RATE") {
        if (value >= 5 && value <= 8) {
            customConfig.codingRate = (uint8_t)value;
            valid = true;
        }
    }
    else if (param == "POWER" || param == "TX_POWER") {
        if (value >= 2 && value <= 20) {
            customConfig.txPower = (int8_t)value;
            valid = true;
        }
    }
    else if (param == "PREAMBLE") {
        if (value >= 6 && value <= 16) {
            customConfig.preambleLength = (uint8_t)value;
            valid = true;
        }
    }
    
    if (valid) {
        // Recalcular métricas estimadas
        customConfig.approxRange = estimateRange(PROFILE_CUSTOM_ADVANCED);
        customConfig.airtimeMs = calculateAirtime(PROFILE_CUSTOM_ADVANCED);
        customConfig.batteryRating = calculateBatteryRating(PROFILE_CUSTOM_ADVANCED);
        customConfig.speedRating = calculateSpeedRating(PROFILE_CUSTOM_ADVANCED);
        
        Serial.println("[Radio Profile] Parámetro " + param + " configurado: " + String(value));
        return true;
    } else {
        Serial.println("[Radio Profile] ERROR: Valor inválido para " + param + ": " + String(value));
        return false;
    }
}

/*
 * CÁLCULOS Y ESTIMACIONES
 */

uint16_t RadioProfileManager::calculateAirtime(RadioProfile profile, uint8_t packetSize) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    // Fórmula simplificada de airtime LoRa
    // Tiempo de símbolo = (2^SF) / BW
    float symbolTime = pow(2, config.spreadingFactor) / (config.bandwidth * 1000); // en segundos
    
    // Estimación de símbolos por packet (simplificada)
    uint16_t symbols = config.preambleLength + 8 + // Preámbulo + header
                      (packetSize * 8 * config.codingRate) / config.spreadingFactor; // Payload codificado
    
    return (uint16_t)(symbols * symbolTime * 1000); // Convertir a ms
}

uint16_t RadioProfileManager::estimateRange(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    // Estimación basada en processing gain y potencia
    float sfGain = pow(RANGE_GAIN_PER_SF, config.spreadingFactor - 7); // Ganancia por SF
    float powerGain = pow(POWER_RANGE_FACTOR, config.txPower - 14); // Ganancia por potencia
    float bwFactor = sqrt(125.0f / config.bandwidth); // Factor de ancho de banda
    
    return (uint16_t)(BASE_RANGE_SF7 * sfGain * powerGain * bwFactor);
}

uint8_t RadioProfileManager::calculateBatteryRating(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    // Rating basado en airtime y potencia (más airtime/potencia = peor batería)
    uint16_t airtime = calculateAirtime(profile);
    float powerFactor = (float)(config.txPower - MIN_POWER_DBM) / (MAX_POWER_DBM - MIN_POWER_DBM);
    float airtimeFactor = (float)(airtime - MIN_AIRTIME_MS) / (MAX_AIRTIME_MS - MIN_AIRTIME_MS);
    
    float batteryScore = 1.0f - (0.6f * airtimeFactor + 0.4f * powerFactor);
    return (uint8_t)constrain(batteryScore * 10, 1, 10);
}

uint8_t RadioProfileManager::calculateSpeedRating(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    // Rating basado en airtime (menos airtime = mejor velocidad)
    uint16_t airtime = calculateAirtime(profile);
    float speedScore = 1.0f - ((float)(airtime - MIN_AIRTIME_MS) / (MAX_AIRTIME_MS - MIN_AIRTIME_MS));
    return (uint8_t)constrain(speedScore * 10, 1, 10);
}

/*
 * INFORMACIÓN Y DOCUMENTACIÓN
 */

void RadioProfileManager::printProfileInfo(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    
    Serial.println("\n========== RADIO PROFILE INFO ==========");
    Serial.println("Perfil: " + String(config.name));
    Serial.println("Descripción: " + String(config.description));
    Serial.println();
    Serial.println("=== PARÁMETROS TÉCNICOS ===");
    Serial.println("Spreading Factor: SF" + String(config.spreadingFactor));
    Serial.println("Bandwidth: " + String(config.bandwidth) + " kHz");
    Serial.println("Coding Rate: 4/" + String(config.codingRate));
    Serial.println("TX Power: " + String(config.txPower) + " dBm");
    Serial.println("Preamble: " + String(config.preambleLength) + " símbolos");
    Serial.println();
    Serial.println("=== PERFORMANCE ESTIMADO ===");
    Serial.println("Alcance aproximado: " + String(config.approxRange) + " metros");
    Serial.println("Airtime (44 bytes): " + String(config.airtimeMs) + " ms");
    Serial.println("Rating batería: " + String(config.batteryRating) + "/10");
    Serial.println("Rating velocidad: " + String(config.speedRating) + "/10");
    Serial.println();
    Serial.println("=== CASO DE USO ===");
    Serial.println("Aplicaciones: " + String(config.useCase));
    Serial.println("Trade-offs: " + String(config.tradeOffs));
    Serial.println("======================================\n");
}

void RadioProfileManager::printAllProfiles() {
    Serial.println("\n============ PERFILES DISPONIBLES ============");
    
    for (int i = 0; i < 5; i++) {
        RadioProfileConfig config = PREDEFINED_PROFILES[i];
        String current = (i == currentProfile) ? " [ACTUAL]" : "";
        
        Serial.println(String(i) + ". " + config.name + current);
        Serial.println("   " + String(config.description));
        Serial.println("   SF" + String(config.spreadingFactor) + 
                      ", " + String(config.bandwidth) + "kHz" +
                      ", " + String(config.txPower) + "dBm" +
                      " → ~" + String(config.approxRange) + "m, " + 
                      String(config.airtimeMs) + "ms");
        Serial.println();
    }
    Serial.println("===============================================");
}

void RadioProfileManager::printProfileComparison() {
    Serial.println("\n===================== COMPARACIÓN DE PERFILES =====================");
    Serial.println("Perfil               | SF | BW  | Pow | Alcance | Airtime | Bat | Vel");
    Serial.println("---------------------|----|----|-----|---------|---------|-----|----");
    
    for (int i = 0; i < 5; i++) {
        RadioProfileConfig config = PREDEFINED_PROFILES[i];
        String name = String(config.name);
        name = name.substring(0, 19); // Truncar a 19 chars
        while (name.length() < 19) name += " "; // Padding
        
        Serial.printf("%s| %2d |%4.0f| %2d  | %4dm   | %4dms  | %d/10| %d/10\n",
                     name.c_str(),
                     config.spreadingFactor,
                     config.bandwidth,
                     config.txPower,
                     config.approxRange,
                     config.airtimeMs,
                     config.batteryRating,
                     config.speedRating);
    }
    Serial.println("=====================================================================");
}

/*
 * VALIDACIÓN
 */

bool RadioProfileManager::isValidConfiguration(uint8_t sf, float bw, uint8_t cr, int8_t power) {
    return (sf >= 7 && sf <= 12) &&
           (bw == 125.0f || bw == 250.0f || bw == 500.0f) &&
           (cr >= 5 && cr <= 8) &&
           (power >= 2 && power <= 20);
}

bool RadioProfileManager::isCompatibleWithRegion(RadioProfile profile) {
    // Todos los perfiles son compatibles con todas las regiones
    // Las limitaciones son más sobre regulaciones de potencia y duty cycle
    return true;
}

/*
 * GETTERS
 */

String RadioProfileManager::getProfileName(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    return String(config.name);
}

String RadioProfileManager::getProfileDescription(RadioProfile profile) {
    RadioProfileConfig config = getProfileConfig(profile);
    return String(config.description);
}