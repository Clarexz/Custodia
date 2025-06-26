/*
 * LORA_HARDWARE.CPP - Inicialización y Configuración del Hardware SX1262
 * 
 * Este archivo contiene todas las funciones relacionadas con la
 * inicialización del hardware y configuración de parámetros del radio.
 */

#include "../lora.h"
#include "../config.h"

// Instancia global del LoRaManager
LoRaManager loraManager;

/*
 * CONSTRUCTOR
 * 
 * Inicializa el objeto SX1262 con los pines correctos para XIAO ESP32S3
 */
LoRaManager::LoRaManager() : radio(new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_NRST_PIN, LORA_BUSY_PIN)) {
    // Inicializar estado existente
    status = LORA_STATUS_INIT;
    deviceID = 0;
    packetCounter = 0;
    
    // Inicializar estadísticas (enhanced)
    stats.packetsSent = 0;
    stats.packetsReceived = 0;
    stats.packetsLost = 0;
    stats.lastRSSI = 0.0f;
    stats.lastSNR = 0.0f;
    stats.totalAirTime = 0;
    
    // NUEVAS estadísticas mesh
    stats.duplicatesIgnored = 0;
    stats.rebroadcasts = 0;
    stats.hopLimitReached = 0;
    
    // Inicializar mesh components
    currentRole = ROLE_NONE;
    recentBroadcasts.reserve(MAX_RECENT_PACKETS);
}

/*
 * DESTRUCTOR
 */
LoRaManager::~LoRaManager() {
    // Cleanup si es necesario
}

/*
 * INICIALIZACIÓN PRINCIPAL DEL SISTEMA LORA
 */
bool LoRaManager::begin() {
    return begin(1);  // Device ID por defecto
}

bool LoRaManager::begin(uint16_t devID) {
    Serial.println("[LoRa] Inicializando sistema LoRa...");
    
    // Establecer device ID
    deviceID = devID;
    
    // Obtener role desde ConfigManager si está disponible
    if (configManager.getConfig().role != ROLE_NONE) {
        currentRole = configManager.getConfig().role;
        Serial.println("[LoRa] Role obtenido de config: " + String(currentRole));
    }
    
    // Inicializar hardware
    if (!initRadio()) {
        Serial.println("[LoRa] ERROR: Fallo en inicialización de hardware");
        status = LORA_STATUS_ERROR;
        return false;
    }
    
    // Configurar parámetros de radio
    if (!configureRadio()) {
        Serial.println("[LoRa] ERROR: Fallo en configuración de radio");
        status = LORA_STATUS_ERROR;
        return false;
    }
    
    // Test básico de funcionamiento
    if (!selfTest()) {
        Serial.println("[LoRa] WARNING: Self-test falló, pero continuando...");
    }
    
    // Configurar modo de recepción inicial
    int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: No se pudo iniciar modo recepción");
        Serial.println("[LoRa] Error code: " + String(state));
        status = LORA_STATUS_ERROR;
        return false;
    }
    
    status = LORA_STATUS_READY;
    Serial.println("[LoRa] Sistema LoRa inicializado exitosamente");
    Serial.println("[LoRa] Device ID: " + String(deviceID));
    Serial.println("[LoRa] Role: " + String(currentRole));
    Serial.println("[LoRa] Frecuencia: " + String(LORA_FREQUENCY) + " MHz");
    Serial.println("[LoRa] Algoritmo Meshtastic: ACTIVADO");
    
    return true;
}

/*
 * INICIALIZACIÓN DEL HARDWARE SX1262
 */
bool LoRaManager::initRadio() {
    Serial.println("[LoRa] Inicializando módulo SX1262...");
    
    // Configurar SPI
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);
    
    // Inicializar módulo SX1262
    int state = radio.begin();
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Módulo SX1262 inicializado correctamente");
        return true;
    } else {
        Serial.println("[LoRa] ERROR: Fallo en inicialización SX1262");
        Serial.println("[LoRa] Error code: " + String(state));
        return false;
    }
}

/*
 * CONFIGURACIÓN DE PARÁMETROS DE RADIO
 */
bool LoRaManager::configureRadio() {
    Serial.println("[LoRa] Configurando parámetros de radio...");
    
    int state;
    
    // Configurar frecuencia
    state = radio.setFrequency(LORA_FREQUENCY);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando frecuencia");
        return false;
    }
    
    // Configurar potencia de transmisión
    state = radio.setOutputPower(LORA_TX_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando potencia TX");
        return false;
    }
    
    // Configurar bandwidth
    state = radio.setBandwidth(LORA_BANDWIDTH);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando bandwidth");
        return false;
    }
    
    // Configurar spreading factor
    state = radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando spreading factor");
        return false;
    }
    
    // Configurar coding rate
    state = radio.setCodingRate(LORA_CODING_RATE);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando coding rate");
        return false;
    }
    
    // Configurar sync word
    state = radio.setSyncWord(LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando sync word");
        return false;
    }
    
    // Configurar longitud de preámbulo
    state = radio.setPreambleLength(LORA_PREAMBLE_LENGTH);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] ERROR: Fallo configurando preámbulo");
        return false;
    }
    
    Serial.println("[LoRa] Configuración de radio completada");
    return true;
}

/*
 * MÉTODOS DE CONFIGURACIÓN EN TIEMPO REAL
 */
void LoRaManager::setFrequency(float frequency) {
    if (radio.setFrequency(frequency) == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Frecuencia cambiada a: " + String(frequency) + " MHz");
    }
}

void LoRaManager::setTxPower(int8_t power) {
    if (radio.setOutputPower(power) == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Potencia TX cambiada a: " + String(power) + " dBm");
    }
}

void LoRaManager::setBandwidth(float bandwidth) {
    if (radio.setBandwidth(bandwidth) == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Bandwidth cambiado a: " + String(bandwidth) + " kHz");
    }
}

void LoRaManager::setSpreadingFactor(uint8_t sf) {
    if (radio.setSpreadingFactor(sf) == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Spreading Factor cambiado a: SF" + String(sf));
    }
}

/*
 * FUNCIONES DE CONTROL DE ENERGÍA
 */
void LoRaManager::sleep() {
    Serial.println("[LoRa] Entrando en modo sleep...");
    radio.sleep();
    status = LORA_STATUS_INIT;  // Requerirá re-inicialización
}

void LoRaManager::wakeup() {
    Serial.println("[LoRa] Despertando del sleep...");
    // Re-inicializar configuración básica
    configureRadio();
    radio.startReceive();
    status = LORA_STATUS_READY;
}

void LoRaManager::reset() {
    Serial.println("[LoRa] Reseteando módulo LoRa...");
    
    // Reset por hardware
    digitalWrite(LORA_NRST_PIN, LOW);
    delay(10);
    digitalWrite(LORA_NRST_PIN, HIGH);
    delay(100);
    
    // Re-inicializar
    initRadio();
    configureRadio();
    radio.startReceive();
    status = LORA_STATUS_READY;
    
    Serial.println("[LoRa] Reset completado");
}

/*
 * TEST BÁSICO DE FUNCIONAMIENTO
 */
bool LoRaManager::selfTest() {
    Serial.println("[LoRa] Ejecutando self-test...");
    
    // Test básico: verificar si podemos leer registros
    int state = radio.standby();
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Self-test PASSED: Comunicación SPI OK");
        return true;
    } else {
        Serial.println("[LoRa] Self-test FAILED: Error en comunicación SPI");
        Serial.println("[LoRa] Error code: " + String(state));
        return false;
    }
}