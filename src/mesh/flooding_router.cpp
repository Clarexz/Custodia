/*
 * FLOODING_ROUTER.CPP - Implementación del Algoritmo Meshtastic
 * 
 * Implementa el algoritmo completo de Managed Flood Routing
 * copiado de Meshtastic FloodingRouter.cpp y RadioInterface.cpp
 */

#include "flooding_router.h"
#include <algorithm>

/*
 * INSTANCIA GLOBAL
 */
FloodingRouter floodingRouter;

/*
 * CONSTRUCTOR
 */
FloodingRouter::FloodingRouter() {
    // Inicializar configuración por defecto
    currentRole = ROLE_NONE;
    deviceID = 0;
    rebroadcastEnabled = true;
    maxHops = MESHTASTIC_MAX_HOPS;
    
    // Configuración personalizada deshabilitada por defecto
    useCustomCW = false;
    useCustomSNR = false;
    
    // Inicializar estadísticas
    resetStats();
    
    Serial.println("[FloodingRouter] Inicializado con algoritmo Meshtastic");
}

/*
 * DESTRUCTOR
 */
FloodingRouter::~FloodingRouter() {
    // Cleanup si es necesario
}

/*
 * INICIALIZACIÓN Y CONFIGURACIÓN
 */
void FloodingRouter::begin(uint16_t devID, DeviceRole role) {
    deviceID = devID;
    currentRole = role;
    
    Serial.println("[FloodingRouter] Configurado - Device ID: " + String(deviceID) + 
                   ", Role: " + String(currentRole));
    Serial.println("[FloodingRouter] Algoritmo: Meshtastic Managed Flood Routing");
    Serial.println("[FloodingRouter] Priority: " + String(getRolePriorityString(role)));
}

void FloodingRouter::setRole(DeviceRole role) {
    currentRole = role;
    Serial.println("[FloodingRouter] Role cambiado a: " + String(role) + 
                   " (Priority: " + String(getRolePriorityString(role)) + ")");
}

void FloodingRouter::setMaxHops(uint8_t hops) {
    maxHops = constrain(hops, 1, 7);  // Límite de Meshtastic
    Serial.println("[FloodingRouter] Max hops configurado: " + String(maxHops));
}

void FloodingRouter::setRebroadcastEnabled(bool enabled) {
    rebroadcastEnabled = enabled;
    Serial.println("[FloodingRouter] Rebroadcast: " + String(enabled ? "HABILITADO" : "DESHABILITADO"));
}

/*
 * ALGORITMO PRINCIPAL - shouldFilterReceived()
 * Implementación exacta de Meshtastic FloodingRouter::shouldFilterReceived()
 */
bool FloodingRouter::shouldFilterReceived(const LoRaPacket* packet) {
    if (!packet) return true;
    
    // Usar el PacketManager para duplicate detection
    if (packetManager.wasSeenRecently(packet)) {
        stats.duplicatesIgnored++;
        onDuplicateDetected(packet);
        
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] Packet duplicado filtrado (Source:" + 
                      String(packet->sourceID) + ", ID:" + String(packet->packetID) + ")");
        #endif
        
        return true;  // Filtrar packet duplicado
    }
    
    // Agregar a packets recientes para futuras verificaciones
    packetManager.addToRecentPackets(packet);
    
    return false;  // No filtrar, packet nuevo
}

/*
 * ALGORITMO PRINCIPAL - perhapsRebroadcast()
 * Implementación de Meshtastic FloodingRouter::perhapsRebroadcast()
 */
bool FloodingRouter::perhapsRebroadcast(const LoRaPacket* packet, float snr) {
    if (!packet) return false;
    
    // Verificaciones básicas (lógica de Meshtastic)
    if (isToUs(packet) || isFromUs(packet)) {
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] No rebroadcast: packet para/de nosotros");
        #endif
        return false;
    }
    
    if (isHopLimitReached(packet)) {
        stats.hopLimitReached++;
        onHopLimitReached(packet);
        return false;
    }
    
    if (!isValidPacketID(packet->packetID)) {
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] No rebroadcast: packet ID inválido");
        #endif
        return false;
    }
    
    if (!isRebroadcaster()) {
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] No rebroadcast: dispositivo no es rebroadcaster");
        #endif
        return false;
    }
    
    // Calcular delay basado en SNR y role (algoritmo de Meshtastic)
    uint32_t delay = getTxDelayMsecWeighted(snr, currentRole);
    
    #if FLOODING_ROUTER_DEBUG
    Serial.println("[FloodingRouter] Rebroadcast programado:");
    Serial.println("  SNR: " + String(snr) + " dB");
    Serial.println("  Role: " + String(currentRole));
    Serial.println("  Delay: " + String(delay) + " ms");
    Serial.println("  Hops: " + String(packet->hops) + "/" + String(packet->maxHops));
    #endif
    
    // Actualizar estadísticas
    stats.rebroadcasts++;
    onRebroadcast(packet, delay);
    
    return true;  // Rebroadcast aprobado
}

/*
 * VERIFICAR SI DISPOSITIVO PUEDE RETRANSMITIR
 * Basado en FloodingRouter::isRebroadcaster()
 */
bool FloodingRouter::isRebroadcaster() const {
    //!En Meshtastic, todos los roles excepto CLIENT_MUTE pueden retransmitir
    //!En nuestro sistema, todos los roles configurados pueden retransmitir
    return (currentRole != ROLE_NONE && rebroadcastEnabled);
}

/*
 * SNR-BASED DELAYS - Implementación exacta de RadioInterface.cpp
 */
uint8_t FloodingRouter::getCWsize(float snr) const {
    // Usar configuración personalizada si está habilitada
    int32_t snrMin = useCustomSNR ? customSNRmin : ContentionWindow::SNR_MIN;
    int32_t snrMax = useCustomSNR ? customSNRmax : ContentionWindow::SNR_MAX;
    uint8_t cwMin = useCustomCW ? customCWmin : ContentionWindow::CWmin;
    uint8_t cwMax = useCustomCW ? customCWmax : ContentionWindow::CWmax;
    
    // Mapear SNR al tamaño de contention window
    // SNR bajo = CW pequeño (delay corto, prioridad alta)
    // SNR alto = CW grande (delay largo, prioridad baja)
    return mapValue(snr, snrMin, snrMax, cwMin, cwMax);
}

uint32_t FloodingRouter::getTxDelayMsecWeighted(float snr, DeviceRole role) const {
    uint8_t CWsize = getCWsize(snr);
    uint32_t delay = 0;
    
    // Algoritmo de Meshtastic RadioInterface::getTxDelayMsecWeighted()
    uint16_t slotTime = useCustomCW ? customSlotTime : ContentionWindow::slotTimeMsec;
    uint8_t cwMax = useCustomCW ? customCWmax : ContentionWindow::CWmax;
    
    if (hasRolePriority(role)) {
        // ROUTERS/REPEATERS tienen MENOS delay (mayor prioridad)
        delay = getRandomDelay(CWsize) * slotTime;
        
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] ROUTER delay: " + String(delay) + " ms (CWsize:" + String(CWsize) + ")");
        #endif
    } else {
        // CLIENTS (TRACKER/RECEIVER) tienen MÁS delay
        delay = (2 * cwMax * slotTime) + (getRandomDelay(CWsize) * slotTime);
        
        #if FLOODING_ROUTER_DEBUG
        Serial.println("[FloodingRouter] CLIENT delay: " + String(delay) + " ms (CWsize:" + String(CWsize) + ")");
        #endif
    }
    
    // Aplicar límites de seguridad
    delay = constrain(delay, FLOODING_MIN_DELAY_MS, FLOODING_MAX_DELAY_MS);
    
    return delay;
}

uint32_t FloodingRouter::getRandomDelay(uint8_t cwSize) const {
    // Generar número aleatorio en rango [0, 2^cwSize)
    uint32_t maxDelay = powerOf2(cwSize);
    return random(0, maxDelay);
}

/*
 * PACKET VALIDATION Y ROUTING LOGIC
 */
bool FloodingRouter::isToUs(const LoRaPacket* packet) const {
    return (packet->destinationID == deviceID || isBroadcast(packet->destinationID));
}

bool FloodingRouter::isFromUs(const LoRaPacket* packet) const {
    return (packet->sourceID == deviceID);
}

bool FloodingRouter::isBroadcast(uint16_t destinationID) const {
    return isBroadcastAddress(destinationID);
}

bool FloodingRouter::canRebroadcast(const LoRaPacket* packet) const {
    return !isHopLimitReached(packet) && isValidPacketID(packet->packetID);
}

bool FloodingRouter::hasRolePriority(DeviceRole role) const {
    // En Meshtastic, ROUTER y REPEATER tienen prioridad alta
    return (role == ROLE_REPEATER);
}

/*
 * HOP MANAGEMENT
 */
bool FloodingRouter::isHopLimitReached(const LoRaPacket* packet) const {
    return (packet->hops >= packet->maxHops);
}

void FloodingRouter::incrementHopCount(LoRaPacket* packet) const {
    if (packet && packet->hops < packet->maxHops) {
        packet->hops++;
    }
}

bool FloodingRouter::isValidForRouting(const LoRaPacket* packet) const {
    return (packet && 
            isValidPacketID(packet->packetID) && 
            !isHopLimitReached(packet) && 
            !isFromUs(packet));
}

/*
 * ESTADÍSTICAS Y DIAGNÓSTICO
 */
void FloodingRouter::resetStats() {
    stats.duplicatesIgnored = 0;
    stats.rebroadcasts = 0;
    stats.hopLimitReached = 0;
    stats.packetsMemory = 0;
    Serial.println("[FloodingRouter] Estadísticas reseteadas");
}

void FloodingRouter::printConfiguration() const {
    Serial.println("\n[FloodingRouter] === CONFIGURACIÓN ===");
    Serial.println("Device ID: " + String(deviceID));
    Serial.println("Role: " + String(currentRole) + " (" + String(getRolePriorityString(currentRole)) + ")");
    Serial.println("Max Hops: " + String(maxHops));
    Serial.println("Rebroadcast: " + String(rebroadcastEnabled ? "HABILITADO" : "DESHABILITADO"));
    Serial.println("Algoritmo: Meshtastic Managed Flood Routing");
    
    // Configuración de Contention Window
    if (useCustomCW) {
        Serial.println("CW personalizado: " + String(customCWmin) + "/" + String(customCWmax) + 
                      " (slot: " + String(customSlotTime) + "ms)");
    } else {
        Serial.println("CW Meshtastic: " + String(ContentionWindow::CWmin) + "/" + 
                      String(ContentionWindow::CWmax) + " (slot: " + String(ContentionWindow::slotTimeMsec) + "ms)");
    }
    
    // Configuración de SNR
    if (useCustomSNR) {
        Serial.println("SNR personalizado: " + String(customSNRmin) + " a " + String(customSNRmax) + " dB");
    } else {
        Serial.println("SNR Meshtastic: " + String(ContentionWindow::SNR_MIN) + " a " + 
                      String(ContentionWindow::SNR_MAX) + " dB");
    }
    
    Serial.println("============================");
}

void FloodingRouter::printMeshStats() const {
    Serial.println("\n[FloodingRouter] === ESTADÍSTICAS MESH ===");
    Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
    Serial.println("Rebroadcasts realizados: " + String(stats.rebroadcasts));
    Serial.println("Hop limit alcanzado: " + String(stats.hopLimitReached));
    Serial.println("Packets en memoria: " + String(packetManager.getPacketsInMemory()));
    Serial.println("Uso de memoria: " + String(packetManager.getMemoryUsage()) + "%");
    Serial.println("Tasa duplicados: " + String(packetManager.getDuplicateRate(), 2) + "%");
    Serial.println("===========================");
}

void FloodingRouter::printPacketInfo(const LoRaPacket* packet) const {
    if (!packet) return;
    
    Serial.println("\n[FloodingRouter] === PACKET INFO ===");
    Serial.println("Tipo: " + String(getMessageTypeString((LoRaMessageType)packet->messageType)));
    Serial.println("Source: " + String(packet->sourceID) + (isFromUs(packet) ? " (NOSOTROS)" : ""));
    Serial.println("Destination: " + String(packet->destinationID) + (isToUs(packet) ? " (PARA NOSOTROS)" : ""));
    Serial.println("Hops: " + String(packet->hops) + "/" + String(packet->maxHops));
    Serial.println("Packet ID: " + String(packet->packetID));
    Serial.println("Payload: " + String(packet->payloadLength) + " bytes");
    Serial.println("Valid for routing: " + String(isValidForRouting(packet) ? "SÍ" : "NO"));
    Serial.println("Can rebroadcast: " + String(canRebroadcast(packet) ? "SÍ" : "NO"));
    Serial.println("=======================");
}

/*
 * CONFIGURACIÓN AVANZADA
 */
void FloodingRouter::setContentionWindow(uint8_t cwMin, uint8_t cwMax, uint16_t slotTime) {
    customCWmin = cwMin;
    customCWmax = cwMax;
    customSlotTime = slotTime;
    useCustomCW = true;
    
    Serial.println("[FloodingRouter] CW personalizado: " + String(cwMin) + "/" + String(cwMax) + 
                   " (slot: " + String(slotTime) + "ms)");
}

void FloodingRouter::setSNRRange(int32_t snrMin, int32_t snrMax) {
    customSNRmin = snrMin;
    customSNRmax = snrMax;
    useCustomSNR = true;
    
    Serial.println("[FloodingRouter] SNR personalizado: " + String(snrMin) + " a " + String(snrMax) + " dB");
}

/*
 * CALLBACKS Y EVENTOS
 */
void FloodingRouter::onDuplicateDetected(const LoRaPacket* packet) {
    // Callback cuando se detecta duplicado
    #if FLOODING_ROUTER_DEBUG
    Serial.println("[FloodingRouter] EVENTO: Duplicado detectado de " + String(packet->sourceID));
    #endif
}

void FloodingRouter::onRebroadcast(const LoRaPacket* packet, uint32_t delay) {
    // Callback cuando se programa rebroadcast
    #if FLOODING_ROUTER_DEBUG
    Serial.println("[FloodingRouter] EVENTO: Rebroadcast programado con delay " + String(delay) + "ms");
    #endif
}

void FloodingRouter::onHopLimitReached(const LoRaPacket* packet) {
    // Callback cuando se alcanza hop limit
    #if FLOODING_ROUTER_DEBUG
    Serial.println("[FloodingRouter] EVENTO: Hop limit alcanzado para packet " + String(packet->packetID));
    #endif
}

/*
 * MÉTODOS PRIVADOS
 */
void FloodingRouter::updateStats(const char* event) {
    // Actualizar estadísticas según el evento
    stats.packetsMemory = packetManager.getPacketsInMemory();
}

bool FloodingRouter::validateConfiguration() const {
    return (deviceID != 0 && currentRole != ROLE_NONE && maxHops > 0);
}

float FloodingRouter::getRolePriorityMultiplier(DeviceRole role) const {
    switch (role) {
        case ROLE_REPEATER: return ROUTER_PRIORITY_MULTIPLIER;
        case ROLE_TRACKER:
        case ROLE_RECEIVER:
        default: return CLIENT_PRIORITY_MULTIPLIER;
    }
}

/*
 * UTILIDADES Y HELPERS
 */
long mapValue(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint32_t generateRandomDelay(uint32_t minDelay, uint32_t maxDelay) {
    if (minDelay >= maxDelay) return minDelay;
    return random(minDelay, maxDelay + 1);
}

uint32_t powerOf2(uint8_t exponent) {
    return (1 << exponent);
}

bool validatePacketFormat(const LoRaPacket* packet) {
    return (packet && 
            packet->payloadLength <= LORA_MAX_PAYLOAD_SIZE &&
            isValidPacketID(packet->packetID));
}

const char* getRoutingAlgorithmInfo() {
    return "Meshtastic Managed Flood Routing v2.0 - Implementación completa con SNR-based delays y role priority";
}