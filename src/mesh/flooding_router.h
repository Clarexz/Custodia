/*
 * FLOODING_ROUTER.H - Algoritmo de Managed Flood Routing de Meshtastic
 * 
 * Este módulo implementa el algoritmo completo de Managed Flood Routing
 * basado exactamente en el código de Meshtastic FloodingRouter.cpp y
 * RadioInterface.cpp para SNR-based delays.
 */

#ifndef FLOODING_ROUTER_H
#define FLOODING_ROUTER_H

#include "mesh_types.h"
#include "packet_manager.h"

/*
 * CLASE PRINCIPAL - FloodingRouter
 * 
 * Implementa el algoritmo completo de Meshtastic:
 * - SNR-based intelligent delays
 * - Role-based priority
 * - Smart rebroadcast logic
 * - Hop management
 */
class FloodingRouter {
private:
    // Role actual del dispositivo para mesh priority
    DeviceRole currentRole;
    uint16_t deviceID;
    
    // Contention Window para SNR-based delays
    ContentionWindow cw;
    
    // Estadísticas específicas de flooding
    MeshStats stats;
    
    // Configuración
    bool rebroadcastEnabled;
    uint8_t maxHops;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    FloodingRouter();
    ~FloodingRouter();
    
    /*
     * INICIALIZACIÓN Y CONFIGURACIÓN
     */
    
    void begin(uint16_t deviceID, DeviceRole role);
    void setRole(DeviceRole role);
    void setMaxHops(uint8_t hops);
    void setRebroadcastEnabled(bool enabled);
    
    /*
     * MÉTODOS PRINCIPALES DEL ALGORITMO MESHTASTIC
     */
    
    // Verificar si un packet debe ser filtrado (duplicate detection)
    // Basado en FloodingRouter::shouldFilterReceived()
    bool shouldFilterReceived(const LoRaPacket* packet);
    
    // Lógica principal de rebroadcast
    // Implementación de FloodingRouter::perhapsRebroadcast()
    bool perhapsRebroadcast(const LoRaPacket* packet, float snr);
    
    // Verificar si el dispositivo puede retransmitir
    // Basado en FloodingRouter::isRebroadcaster()
    bool isRebroadcaster() const;
    
    /*
     * SNR-BASED DELAYS - Algoritmo de RadioInterface.cpp
     */
    
    // Calcular tamaño de contention window basado en SNR
    // Implementación de RadioInterface::getCWsize()
    uint8_t getCWsize(float snr) const;
    
    // Calcular delay ponderado basado en SNR y role
    // Implementación de RadioInterface::getTxDelayMsecWeighted()
    uint32_t getTxDelayMsecWeighted(float snr, DeviceRole role) const;
    
    // Calcular delay aleatorio para contention window
    uint32_t getRandomDelay(uint8_t cwSize) const;
    
    /*
     * PACKET VALIDATION Y ROUTING LOGIC
     */
    
    // Verificar si el packet es para nosotros
    bool isToUs(const LoRaPacket* packet) const;
    
    // Verificar si el packet es de nosotros
    bool isFromUs(const LoRaPacket* packet) const;
    
    // Verificar si es dirección de broadcast
    bool isBroadcast(uint16_t destinationID) const;
    
    // Verificar si el packet puede ser retransmitido (hop limit)
    bool canRebroadcast(const LoRaPacket* packet) const;
    
    // Verificar si el role tiene prioridad especial
    bool hasRolePriority(DeviceRole role) const;
    
    /*
     * HOP MANAGEMENT
     */
    
    // Verificar si el packet ha alcanzado el hop limit
    bool isHopLimitReached(const LoRaPacket* packet) const;
    
    // Incrementar hop count en un packet
    void incrementHopCount(LoRaPacket* packet) const;
    
    // Verificar si el packet es válido para routing
    bool isValidForRouting(const LoRaPacket* packet) const;
    
    /*
     * ESTADÍSTICAS Y DIAGNÓSTICO
     */
    
    // Obtener estadísticas de mesh
    MeshStats getMeshStats() const { return stats; }
    
    // Obtener estadísticas específicas
    uint32_t getDuplicatesIgnored() const { return stats.duplicatesIgnored; }
    uint32_t getRebroadcasts() const { return stats.rebroadcasts; }
    uint32_t getHopLimitReached() const { return stats.hopLimitReached; }
    
    // Reset estadísticas
    void resetStats();
    
    // Debug y información
    void printConfiguration() const;
    void printMeshStats() const;
    void printPacketInfo(const LoRaPacket* packet) const;
    
    /*
     * CONFIGURACIÓN AVANZADA
     */
    
    // Configurar parámetros de contention window personalizados
    void setContentionWindow(uint8_t cwMin, uint8_t cwMax, uint16_t slotTime);
    
    // Configurar rango de SNR personalizado
    void setSNRRange(int32_t snrMin, int32_t snrMax);
    
    // Obtener configuración actual
    DeviceRole getCurrentRole() const { return currentRole; }
    uint16_t getDeviceID() const { return deviceID; }
    uint8_t getMaxHops() const { return maxHops; }
    bool isRebroadcastEnabled() const { return rebroadcastEnabled; }
    
    /*
     * CALLBACKS Y EVENTOS
     */
    
    // Callback cuando se detecta un packet duplicado
    void onDuplicateDetected(const LoRaPacket* packet);
    
    // Callback cuando se realiza un rebroadcast
    void onRebroadcast(const LoRaPacket* packet, uint32_t delay);
    
    // Callback cuando se alcanza hop limit
    void onHopLimitReached(const LoRaPacket* packet);
    
private:
    /*
     * MÉTODOS PRIVADOS
     */
    
    // Configuración personalizada de CW
    uint8_t customCWmin, customCWmax;
    uint16_t customSlotTime;
    int32_t customSNRmin, customSNRmax;
    bool useCustomCW, useCustomSNR;
    
    // Actualizar estadísticas internas
    void updateStats(const char* event);
    
    // Validar parámetros de configuración
    bool validateConfiguration() const;
    
    // Calcular prioridad de role para delays
    float getRolePriorityMultiplier(DeviceRole role) const;
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en flooding_router.cpp
 */
extern FloodingRouter floodingRouter;

/*
 * UTILIDADES Y HELPERS
 */

// Mapear valor entre rangos (equivalente a Arduino map())
long mapValue(long x, long in_min, long in_max, long out_min, long out_max);

// Generar delay aleatorio con distribución uniforme
uint32_t generateRandomDelay(uint32_t minDelay, uint32_t maxDelay);

// Calcular potencia de 2 eficientemente
uint32_t powerOf2(uint8_t exponent);

// Validar que un packet tenga formato correcto
bool validatePacketFormat(const LoRaPacket* packet);

// Obtener string descriptivo del algoritmo de routing
const char* getRoutingAlgorithmInfo();

/*
 * CONSTANTES ESPECÍFICAS DEL FLOODING ROUTER
 */

// Delays mínimos y máximos recomendados
#define FLOODING_MIN_DELAY_MS    10     // Delay mínimo en ms
#define FLOODING_MAX_DELAY_MS    2000   // Delay máximo en ms

// Multiplicadores de prioridad para roles
#define ROUTER_PRIORITY_MULTIPLIER    0.5f  // REPEATER 50% más rápido
#define CLIENT_PRIORITY_MULTIPLIER    1.0f  // TRACKER/RECEIVER normal

// Configuración de debug
#define FLOODING_ROUTER_DEBUG    1      // Habilitar mensajes de debug

#endif // FLOODING_ROUTER_H