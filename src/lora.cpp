/*
 * LORA.CPP - Implementación del Sistema LoRa Enhanced con Algoritmo Meshtastic
 * 
 * Este archivo implementa toda la funcionalidad del sistema LoRa
 * para comunicación mesh usando el módulo SX1262 con RadioLib,
 * ahora con el algoritmo completo de Managed Flood Routing
 * copiado exactamente del código fuente de Meshtastic.
 * 
 * Autor: Caleb Martinez Cabazos
 */

#include "lora.h"
#include "config.h"

/*
 * INSTANCIA GLOBAL
 */
LoRaManager loraManager;

/*
 * CONSTRUCTOR (NO MODIFICADO)
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
 * DESTRUCTOR (NO MODIFICADO)
 */
LoRaManager::~LoRaManager() {
    // Cleanup si es necesario
}

/*
 * INICIALIZACIÓN PRINCIPAL DEL SISTEMA LORA (NO MODIFICADO)
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
 * INICIALIZACIÓN DEL HARDWARE SX1262 (NO MODIFICADO)
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
 * CONFIGURACIÓN DE PARÁMETROS DE RADIO (NO MODIFICADO)
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
 * LOOP PRINCIPAL DE ACTUALIZACIÓN (ENHANCED)
 */
void LoRaManager::update() {
    // Limpiar packets antiguos cada 30 segundos
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup >= 30000) {
        cleanOldPackets();
        lastCleanup = millis();
    }
    
    // Verificar si hay packets recibidos
    if (isPacketAvailable()) {
        LoRaPacket packet;
        if (receivePacket(&packet)) {
            // El processamiento ya se hace en receivePacket() con mesh logic
        }
    }
}

/*
 * RECEPCIÓN Y PROCESAMIENTO DE PACKETS (ENHANCED WITH MESH LOGIC)
 */
bool LoRaManager::receivePacket(LoRaPacket* packet) {
    if (!packet) return false;
    
    // Recibir datos del radio
    int state = radio.readData((uint8_t*)packet, sizeof(LoRaPacket));
    
    if (state == RADIOLIB_ERR_NONE) {
        // Obtener estadísticas de señal
        stats.lastRSSI = radio.getRSSI();
        stats.lastSNR = radio.getSNR();
        
        // Validar packet básico
        if (!validatePacket(packet)) {
            stats.packetsLost++;
            Serial.println("[LoRa] Packet inválido (checksum)");
            return false;
        }
        
        // === MESHTASTIC ALGORITHM: DUPLICATE DETECTION ===
        if (shouldFilterReceived(packet)) {
            stats.duplicatesIgnored++;
            Serial.println("[LoRa] Packet duplicado ignorado (sourceID=" + String(packet->sourceID) + ", packetID=" + String(packet->packetID) + ")");
            return false;  // Packet duplicado, ignorar
        }
        
        // Packet válido y nuevo
        stats.packetsReceived++;
        Serial.println("[LoRa] Packet válido recibido");
        Serial.println("[LoRa] RSSI: " + String(stats.lastRSSI) + " dBm");
        Serial.println("[LoRa] SNR: " + String(stats.lastSNR) + " dB");
        Serial.println("[LoRa] Source: " + String(packet->sourceID) + ", Hops: " + String(packet->hops) + "/" + String(packet->maxHops));
        
        // Agregar a seen packets para evitar futuras retransmisiones
        addToRecentPackets(packet->sourceID, packet->packetID);
        
        // === MESHTASTIC ALGORITHM: MESH REBROADCAST ===
        if (perhapsRebroadcast(packet)) {
            stats.rebroadcasts++;
            Serial.println("[LoRa] Packet programado para retransmisión");
        }
        
        // Procesar contenido según tipo de mensaje
        switch (packet->messageType) {
            case MSG_GPS_DATA:
                // Procesar datos GPS recibidos
                float lat, lon;
                uint32_t timestamp;
                uint16_t sourceID;
                if (processGPSPacket(packet, &lat, &lon, &timestamp, &sourceID)) {
                    Serial.println("[LoRa] GPS recibido de device " + String(sourceID) + 
                                 ": " + String(lat, 6) + "," + String(lon, 6));
                }
                break;
                
            case MSG_HEARTBEAT:
                Serial.println("[LoRa] Heartbeat recibido de device " + String(packet->sourceID));
                break;
                
            default:
                Serial.println("[LoRa] Packet tipo desconocido: " + String(packet->messageType));
                break;
        }
        
        return true;
        
    } else {
        Serial.println("[LoRa] ERROR: Fallo en recepción");
        Serial.println("[LoRa] Error code: " + String(state));
        return false;
    }
}

/*
 * MESHTASTIC ALGORITHM: DUPLICATE DETECTION
 * Copiado exacto de FloodingRouter.cpp
 */
bool LoRaManager::wasSeenRecently(const LoRaPacket* packet) {
    // Buscar si ya vimos este packet (sourceID + packetID)
    for (const auto& record : recentBroadcasts) {
        if (record.sourceID == packet->sourceID && record.packetID == packet->packetID) {
            return true;  // Ya lo vimos
        }
    }
    return false;  // Packet nuevo
}

void LoRaManager::addToRecentPackets(uint16_t sourceID, uint32_t packetID) {
    // Si ya tenemos demasiados, eliminar el más antiguo
    if (recentBroadcasts.size() >= MAX_RECENT_PACKETS) {
        recentBroadcasts.erase(recentBroadcasts.begin());
    }
    
    // Agregar nuevo record
    PacketRecord record;
    record.sourceID = sourceID;
    record.packetID = packetID;
    record.timestamp = millis();
    
    recentBroadcasts.push_back(record);
}

void LoRaManager::cleanOldPackets() {
    unsigned long currentTime = millis();
    
    // Eliminar packets más antiguos que PACKET_MEMORY_TIME
    recentBroadcasts.erase(
        std::remove_if(recentBroadcasts.begin(), recentBroadcasts.end(),
            [currentTime](const PacketRecord& record) {
                return (currentTime - record.timestamp) > PACKET_MEMORY_TIME;
            }),
        recentBroadcasts.end()
    );
    
    // Debug info
    if (recentBroadcasts.size() > 0) {
        Serial.println("[LoRa] Packets en memoria: " + String(recentBroadcasts.size()));
    }
}

/*
 * MESHTASTIC ALGORITHM: SNR-BASED DELAYS
 * Copiado exacto de RadioInterface.cpp
 */
uint8_t LoRaManager::getCWsize(float snr) {
    // Mapear SNR al tamaño de contention window
    // SNR bajo = CW pequeño (delay corto, prioridad alta)
    // SNR alto = CW grande (delay largo, prioridad baja)
    return map(snr, ContentionWindow::SNR_MIN, ContentionWindow::SNR_MAX, 
               ContentionWindow::CWmin, ContentionWindow::CWmax);
}

uint32_t LoRaManager::getTxDelayMsecWeighted(float snr, DeviceRole role) {
    uint8_t CWsize = getCWsize(snr);
    uint32_t delay = 0;
    
    // EXACT LOGIC FROM MESHTASTIC RadioInterface.cpp
    if (role == ROLE_REPEATER) {  // Como ROUTER en Meshtastic
        // ROUTERS/REPEATERS tienen MENOS delay (mayor prioridad)
        delay = random(0, pow(2, CWsize)) * ContentionWindow::slotTimeMsec;
        Serial.println("[LoRa] REPEATER delay: " + String(delay) + " ms");
    } else {
        // CLIENTS (TRACKER/RECEIVER) tienen MÁS delay
        delay = (2 * ContentionWindow::CWmax * ContentionWindow::slotTimeMsec) + 
                random(0, pow(2, CWsize)) * ContentionWindow::slotTimeMsec;
        Serial.println("[LoRa] CLIENT delay: " + String(delay) + " ms");
    }
    
    return delay;
}

uint32_t LoRaManager::getRandomDelay(uint8_t cwSize) {
    return random(0, pow(2, cwSize)) * ContentionWindow::slotTimeMsec;
}

/*
 * MESHTASTIC ALGORITHM: MESH LOGIC
 * Basado en FloodingRouter.cpp
 */
bool LoRaManager::shouldFilterReceived(const LoRaPacket* packet) {
    // Implementación de shouldFilterReceived de Meshtastic
    if (wasSeenRecently(packet)) {
        return true;  // Filtrar duplicado
    }
    return false;  // Procesar packet
}

bool LoRaManager::isRebroadcaster() {
    // Basado en FloodingRouter::isRebroadcaster()
    // Todos los roles excepto CLIENT_MUTE pueden retransmitir
    // En nuestro caso, todos los roles retransmiten
    return (currentRole != ROLE_NONE);
}

bool LoRaManager::isToUs(const LoRaPacket* packet) {
    // Verificar si el packet es para nosotros
    return (packet->destinationID == deviceID || packet->destinationID == LORA_BROADCAST_ADDR);
}

bool LoRaManager::isFromUs(const LoRaPacket* packet) {
    // Verificar si el packet es de nosotros
    return (packet->sourceID == deviceID);
}

bool LoRaManager::isBroadcast(uint16_t destinationID) {
    return (destinationID == LORA_BROADCAST_ADDR);
}

bool LoRaManager::hasRolePriority(DeviceRole role) {
    // REPEATERS tienen prioridad (como ROUTERS en Meshtastic)
    return (role == ROLE_REPEATER);
}

/*
 * MESHTASTIC ALGORITHM: ENHANCED REBROADCAST
 * Basado exactamente en FloodingRouter::perhapsRebroadcast()
 */
bool LoRaManager::perhapsRebroadcast(const LoRaPacket* packet) {
    // Lógica EXACTA de Meshtastic FloodingRouter::perhapsRebroadcast()
    
    // No retransmitir si es para nosotros, o es de nosotros, o hop limit agotado
    if (isToUs(packet) || isFromUs(packet) || packet->hops >= packet->maxHops) {
        if (packet->hops >= packet->maxHops) {
            stats.hopLimitReached++;
            Serial.println("[LoRa] Packet descartado: hop limit alcanzado (" + String(packet->hops) + "/" + String(packet->maxHops) + ")");
        }
        return false;
    }
    
    // Verificar que packet ID sea válido
    if (packet->packetID == MESHTASTIC_PACKET_ID_INVALID) {
        Serial.println("[LoRa] Packet ignorado: ID inválido");
        return false;
    }
    
    // Verificar si somos rebroadcaster
    if (!isRebroadcaster()) {
        Serial.println("[LoRa] No retransmitir: Role no permite rebroadcast");
        return false;
    }
    
    // Calcular delay basado en SNR y role
    uint32_t meshDelay = getTxDelayMsecWeighted(stats.lastSNR, currentRole);
    
    Serial.println("[LoRa] Programando retransmisión en " + String(meshDelay) + " ms");
    Serial.println("[LoRa] SNR: " + String(stats.lastSNR) + " dB, Role: " + String(currentRole));
    
    // Delay antes de retransmitir (SNR-based)
    delay(meshDelay);
    
    // Crear copia del packet para retransmisión
    LoRaPacket retransmitPacket = *packet;
    retransmitPacket.hops++;  // Incrementar hop count
    
    // Recalcular checksum
    retransmitPacket.checksum = calculateChecksum(&retransmitPacket);
    
    // Cambiar a modo transmisión
    LoRaStatus previousStatus = status;
    status = LORA_STATUS_TRANSMITTING;
    
    // Retransmitir packet
    unsigned long startTime = millis();
    int state = radio.transmit((uint8_t*)&retransmitPacket, sizeof(LoRaPacket));
    unsigned long airTime = millis() - startTime;
    
    stats.totalAirTime += airTime;
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Retransmisión exitosa (hop " + String(retransmitPacket.hops) + ")");
        Serial.println("[LoRa] Air time: " + String(airTime) + " ms");
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return true;
    } else {
        stats.packetsLost++;
        Serial.println("[LoRa] ERROR: Fallo en retransmisión");
        Serial.println("[LoRa] Error code: " + String(state));
        
        // Volver a modo recepción
        radio.startReceive();
        status = previousStatus;
        return false;
    }
}

/*
 * ENVÍO DE DATOS GPS (NO MODIFICADO)
 */
bool LoRaManager::sendGPSData(float latitude, float longitude, uint32_t timestamp) {
    return sendGPSData(latitude, longitude, timestamp, LORA_BROADCAST_ADDR);
}

bool LoRaManager::sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID) {
    // Crear payload GPS
    GPSPayload gpsPayload;
    gpsDataToPayload(latitude, longitude, timestamp, &gpsPayload);
    
    // Enviar como packet GPS
    return sendPacket(MSG_GPS_DATA, (uint8_t*)&gpsPayload, sizeof(GPSPayload), destinationID);
}

/*
 * ENVÍO DE PACKET GENÉRICO (ENHANCED)
 */
bool LoRaManager::sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength) {
    return sendPacket(msgType, payload, payloadLength, LORA_BROADCAST_ADDR);
}

bool LoRaManager::sendPacket(LoRaMessageType msgType, const uint8_t* payload, uint8_t payloadLength, uint16_t destinationID) {
    if (status != LORA_STATUS_READY) {
        Serial.println("[LoRa] ERROR: Sistema no está listo para transmitir");
        return false;
    }
    
    if (payloadLength > LORA_MAX_PAYLOAD_SIZE) {
        Serial.println("[LoRa] ERROR: Payload demasiado grande");
        return false;
    }
    
    // Crear packet
    LoRaPacket packet;
    packet.messageType = msgType;
    packet.sourceID = deviceID;
    packet.destinationID = destinationID;
    packet.hops = 0;  // Packet original
    packet.maxHops = MESHTASTIC_MAX_HOPS;  // Máximo saltos
    packet.packetID = ++packetCounter;
    packet.payloadLength = payloadLength;
    
    // Copiar payload
    memcpy(packet.payload, payload, payloadLength);
    
    // Calcular checksum
    packet.checksum = calculateChecksum(&packet);
    
    // === MESHTASTIC: AGREGAR A SEEN PACKETS ===
    // Agregar nuestros propios packets para evitar retransmitirlos
    addToRecentPackets(packet.sourceID, packet.packetID);
    
    // Cambiar a modo transmisión
    status = LORA_STATUS_TRANSMITTING;
    
    // Transmitir packet
    unsigned long startTime = millis();
    int state = radio.transmit((uint8_t*)&packet, sizeof(LoRaPacket));
    unsigned long airTime = millis() - startTime;
    
    // Actualizar estadísticas
    stats.totalAirTime += airTime;
    
    if (state == RADIOLIB_ERR_NONE) {
        stats.packetsSent++;
        Serial.println("[LoRa] Packet enviado exitosamente");
        Serial.println("[LoRa] PacketID: " + String(packet.packetID) + ", Air time: " + String(airTime) + " ms");
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return true;
    } else {
        stats.packetsLost++;
        Serial.println("[LoRa] ERROR: Fallo en transmisión");
        Serial.println("[LoRa] Error code: " + String(state));
        
        // Volver a modo recepción
        radio.startReceive();
        status = LORA_STATUS_READY;
        return false;
    }
}

/*
 * VERIFICAR SI HAY PACKETS DISPONIBLES (NO MODIFICADO)
 */
bool LoRaManager::isPacketAvailable() {
    // Verificar flag de recepción de RadioLib
    return radio.getIrqStatus() & RADIOLIB_SX126X_IRQ_RX_DONE;
}

/*
 * PROCESAR PACKET GPS RECIBIDO (NO MODIFICADO)
 */
bool LoRaManager::processGPSPacket(const LoRaPacket* packet, float* lat, float* lon, uint32_t* timestamp, uint16_t* sourceID) {
    if (!packet || packet->messageType != MSG_GPS_DATA) return false;
    
    // Extraer payload GPS
    GPSPayload* gpsPayload = (GPSPayload*)packet->payload;
    
    // Convertir datos
    payloadToGpsData(gpsPayload, lat, lon, timestamp);
    *sourceID = packet->sourceID;
    
    return true;
}

/*
 * UTILIDADES PRIVADAS (NO MODIFICADAS)
 */
uint16_t LoRaManager::calculateChecksum(const LoRaPacket* packet) {
    // Checksum simple XOR de todos los bytes excepto el campo checksum
    uint16_t checksum = 0;
    uint8_t* data = (uint8_t*)packet;
    
    // Calcular XOR de todos los bytes excepto los últimos 2 (checksum)
    for (int i = 0; i < sizeof(LoRaPacket) - 2; i++) {
        checksum ^= data[i];
    }
    
    return checksum;
}

bool LoRaManager::validatePacket(const LoRaPacket* packet) {
    // Verificar checksum
    uint16_t calculatedChecksum = calculateChecksum(packet);
    return (calculatedChecksum == packet->checksum);
}

void LoRaManager::gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload) {
    payload->latitude = lat;
    payload->longitude = lon;
    payload->timestamp = timestamp;
    payload->batteryVoltage = 3300;  // 3.3V por defecto (futuro: leer real)
    payload->satellites = 8;         // Simulado
    payload->reserved = 0;
}

void LoRaManager::payloadToGpsData(const GPSPayload* payload, float* lat, float* lon, uint32_t* timestamp) {
    *lat = payload->latitude;
    *lon = payload->longitude;
    *timestamp = payload->timestamp;
}

/*
 * MÉTODOS DE INFORMACIÓN Y DIAGNÓSTICO (ENHANCED)
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

void LoRaManager::printConfiguration() {
    Serial.println("\n[LoRa] === CONFIGURACIÓN ACTUAL ===");
    Serial.println("Device ID: " + String(deviceID));
    Serial.println("Role: " + String(currentRole));
    Serial.println("Frecuencia: " + String(LORA_FREQUENCY) + " MHz");
    Serial.println("TX Power: " + String(LORA_TX_POWER) + " dBm");
    Serial.println("Bandwidth: " + String(LORA_BANDWIDTH) + " kHz");
    Serial.println("Spreading Factor: " + String(LORA_SPREADING_FACTOR));
    Serial.println("Coding Rate: 4/" + String(LORA_CODING_RATE));
    Serial.println("Sync Word: 0x" + String(LORA_SYNC_WORD, HEX));
    Serial.println("Estado: " + getStatusString());
    Serial.println("Algoritmo: Meshtastic Managed Flood Routing");
    Serial.println("================================");
}

void LoRaManager::printStats() {
    Serial.println("\n[LoRa] === ESTADÍSTICAS BÁSICAS ===");
    Serial.println("Packets enviados: " + String(stats.packetsSent));
    Serial.println("Packets recibidos: " + String(stats.packetsReceived));
    Serial.println("Packets perdidos: " + String(stats.packetsLost));
    Serial.println("Último RSSI: " + String(stats.lastRSSI) + " dBm");
    Serial.println("Último SNR: " + String(stats.lastSNR) + " dB");
    Serial.println("Tiempo total aire: " + String(stats.totalAirTime) + " ms");
    Serial.println("=======================");
}

void LoRaManager::printMeshStats() {
    Serial.println("\n[LoRa] === ESTADÍSTICAS MESH ===");
    Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
    Serial.println("Retransmisiones: " + String(stats.rebroadcasts));
    Serial.println("Hop limit alcanzado: " + String(stats.hopLimitReached));
    Serial.println("Packets en memoria: " + String(recentBroadcasts.size()) + "/" + String(MAX_RECENT_PACKETS));
    Serial.println("Role actual: " + String(currentRole));
    Serial.println("CW Min/Max: " + String(ContentionWindow::CWmin) + "/" + String(ContentionWindow::CWmax));
    Serial.println("Slot time: " + String(ContentionWindow::slotTimeMsec) + " ms");
    Serial.println("========================");
}

LoRaStatus LoRaManager::getStatus() {
    return status;
}

String LoRaManager::getStatusString() {
    switch (status) {
        case LORA_STATUS_INIT: return "INICIALIZANDO";
        case LORA_STATUS_READY: return "LISTO";
        case LORA_STATUS_TRANSMITTING: return "TRANSMITIENDO";
        case LORA_STATUS_RECEIVING: return "RECIBIENDO";
        case LORA_STATUS_ERROR: return "ERROR";
        default: return "DESCONOCIDO";
    }
}

LoRaStats LoRaManager::getStats() {
    return stats;
}

float LoRaManager::getLastRSSI() {
    return stats.lastRSSI;
}

float LoRaManager::getLastSNR() {
    return stats.lastSNR;
}

void LoRaManager::resetStats() {
    stats.packetsSent = 0;
    stats.packetsReceived = 0;
    stats.packetsLost = 0;
    stats.totalAirTime = 0;
    stats.duplicatesIgnored = 0;
    stats.rebroadcasts = 0;
    stats.hopLimitReached = 0;
    Serial.println("[LoRa] Estadísticas reseteadas");
}

void LoRaManager::printPacketInfo(const LoRaPacket* packet) {
    Serial.println("\n[LoRa] === INFO DEL PACKET ===");
    Serial.println("Tipo: " + String(packet->messageType));
    Serial.println("Origen: " + String(packet->sourceID));
    Serial.println("Destino: " + String(packet->destinationID));
    Serial.println("Saltos: " + String(packet->hops) + "/" + String(packet->maxHops));
    Serial.println("Packet ID: " + String(packet->packetID));
    Serial.println("Payload: " + String(packet->payloadLength) + " bytes");
    Serial.println("Checksum: 0x" + String(packet->checksum, HEX));
    Serial.println("=========================");
}

/*
 * MÉTODOS DE CONFIGURACIÓN EN TIEMPO REAL (NO MODIFICADOS)
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
 * FUNCIONES DE CONTROL DE ENERGÍA (NO MODIFICADAS)
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