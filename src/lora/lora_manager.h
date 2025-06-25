/*
 * LORA_MANAGER.H - Gestión de Alto Nivel del Sistema LoRa
 * 
 * Este módulo integra el RadioInterface (hardware) con el FloodingRouter (mesh)
 * para proporcionar una interfaz de alto nivel para el sistema LoRa completo.
 * 
 * Responsabilidades:
 * - Coordinar hardware radio y algoritmo mesh
 * - Gestión de packets y protocolos
 * - Interface unificada para el resto del sistema
 * - Callbacks y eventos del sistema
 */

#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include "radio_interface.h"
#include "../mesh/flooding_router.h"
#include "../mesh/packet_manager.h"
#include "../mesh/mesh_types.h"

/*
 * CALLBACKS Y EVENTOS DEL SISTEMA
 */

// Callback para cuando se recibe un packet GPS
typedef void (*OnGPSPacketReceived)(uint16_t sourceID, float lat, float lon, uint32_t timestamp, float rssi, float snr);

// Callback para cuando se recibe cualquier packet
typedef void (*OnPacketReceived)(const LoRaPacket* packet, float rssi, float snr);

// Callback para eventos de mesh
typedef void (*OnMeshEvent)(const char* event, uint16_t nodeID, const char* details);

// Callback para cambios de estado del sistema
typedef void (*OnStatusChanged)(LoRaStatus oldStatus, LoRaStatus newStatus);

/*
 * CONFIGURACIÓN DEL SISTEMA
 */
struct LoRaManagerConfig {
    uint16_t deviceID;
    DeviceRole role;
    bool autoRetransmit;
    bool enableMeshRouting;
    bool enableDuplicateDetection;
    uint8_t maxHops;
    uint32_t packetTimeout;
};

/*
 * CLASE PRINCIPAL - LoRaManager
 * 
 * Interfaz de alto nivel que integra:
 * - RadioInterface (hardware SX1262)
 * - FloodingRouter (algoritmo Meshtastic)
 * - PacketManager (duplicate detection)
 * - Protocolos de aplicación (GPS, heartbeat, etc.)
 */
class LoRaManager {
private:
    // Configuración del sistema
    LoRaManagerConfig config;
    
    // Contador para generar IDs únicos de packets
    uint32_t packetCounter;
    
    // Callbacks registrados
    OnGPSPacketReceived gpsCallback;
    OnPacketReceived packetCallback;
    OnMeshEvent meshCallback;
    OnStatusChanged statusCallback;
    
    // Control de flujo y timing
    unsigned long lastUpdate;
    unsigned long lastStatsReport;
    bool systemInitialized;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    LoRaManager();
    ~LoRaManager();
    
    /*
     * INICIALIZACIÓN Y CONFIGURACIÓN DEL SISTEMA
     */
    
    // Inicializar todo el sistema LoRa
    bool begin();
    bool begin(uint16_t deviceID);
    bool begin(uint16_t deviceID, DeviceRole role);
    
    // Configurar sistema completo
    bool configure(const LoRaManagerConfig& configuration);
    
    // Obtener configuración actual
    LoRaManagerConfig getConfiguration() const { return config; }
    
    /*
     * CONTROL DEL SISTEMA
     */
    
    // Actualización periódica del sistema (llamar en loop)
    void update();
    
    // Reiniciar sistema completo
    bool restart();
    
    // Shutdown del sistema
    void shutdown();
    
    // Verificar estado del sistema
    bool isSystemHealthy();
    
    /*
     * ENVÍO DE DATOS (High Level API)
     */
    
    // Enviar datos GPS (para TRACKER)
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp);
    bool sendGPSData(float latitude, float longitude, uint32_t timestamp, uint16_t destinationID);
    
    // Enviar heartbeat (para todos los roles)
    bool sendHeartbeat();
    bool sendHeartbeat(uint16_t destinationID);
    
    // Enviar comando de configuración remota
    bool sendConfigCommand(uint16_t targetID, const String& command, const String& value);
    
    // Enviar packet personalizado
    bool sendCustomPacket(LoRaMessageType type, const uint8_t* payload, uint8_t length);
    bool sendCustomPacket(LoRaMessageType type, const uint8_t* payload, uint8_t length, uint16_t destinationID);
    
    /*
     * GESTIÓN DE ROLE Y CONFIGURACIÓN
     */
    
    // Cambiar role del dispositivo
    void setRole(DeviceRole role);
    DeviceRole getRole() const { return config.role; }
    
    // Configurar device ID
    void setDeviceID(uint16_t deviceID);
    uint16_t getDeviceID() const { return config.deviceID; }
    
    // Configurar parámetros de mesh
    void setMaxHops(uint8_t hops);
    void setMeshRoutingEnabled(bool enabled);
    void setDuplicateDetectionEnabled(bool enabled);
    
    /*
     * CALLBACKS Y EVENTOS
     */
    
    // Registrar callbacks
    void setGPSCallback(OnGPSPacketReceived callback) { gpsCallback = callback; }
    void setPacketCallback(OnPacketReceived callback) { packetCallback = callback; }
    void setMeshCallback(OnMeshEvent callback) { meshCallback = callback; }
    void setStatusCallback(OnStatusChanged callback) { statusCallback = callback; }
    
    // Limpiar callbacks
    void clearCallbacks();
    
    /*
     * ESTADO Y ESTADÍSTICAS
     */
    
    // Obtener estado del sistema
    LoRaStatus getStatus();
    String getStatusString();
    
    // Obtener estadísticas completas
    LoRaStats getStats();
    MeshStats getMeshStats();
    
    // Obtener calidad de señal
    float getLastRSSI();
    float getLastSNR();
    uint8_t getSignalQuality();  // 0-100%
    
    // Reset de estadísticas
    void resetStats();
    
    /*
     * DIAGNÓSTICO Y DEBUG
     */
    
    // Información completa del sistema
    void printSystemInfo();
    void printConfiguration();
    void printAllStats();
    
    // Test de sistema completo
    bool runSystemTest();
    
    // Test de conectividad con otro nodo
    bool testConnectivity(uint16_t targetID);
    
    // Diagnóstico de problemas
    String getDiagnosticInfo();
    
    /*
     * CONFIGURACIÓN AVANZADA
     */
    
    // Configurar parámetros de radio directamente
    bool setRadioFrequency(float frequency);
    bool setRadioTxPower(int8_t power);
    bool setRadioBandwidth(float bandwidth);
    bool setRadioSpreadingFactor(uint8_t sf);
    
    // Aplicar preset de configuración LoRa
    bool applyLoRaPreset(const LoRaPreset& preset);
    
    // Configuración personalizada de mesh
    bool setCustomContentionWindow(uint8_t cwMin, uint8_t cwMax, uint16_t slotTime);
    bool setCustomSNRRange(int32_t snrMin, int32_t snrMax);
    
    /*
     * UTILIDADES DE PACKET
     */
    
    // Verificar si hay packets disponibles
    bool isPacketAvailable();
    
    // Procesar packet específico manualmente
    bool processPacket(const LoRaPacket* packet, float rssi, float snr);
    
    // Crear packet desde datos
    LoRaPacket createPacket(LoRaMessageType type, const uint8_t* payload, uint8_t length, uint16_t destinationID = LORA_BROADCAST_ADDR);
    
    // Validar packet
    bool validatePacket(const LoRaPacket* packet);
    
    // Calcular checksum de packet
    uint16_t calculateChecksum(const LoRaPacket* packet);
    
private:
    /*
     * MÉTODOS PRIVADOS DE GESTIÓN
     */
    
    // Inicializar componentes individuales
    bool initializeRadio();
    bool initializeMesh();
    bool initializePacketManager();
    
    // Procesamiento de packets recibidos
    void handleReceivedPacket(const LoRaPacket* packet, float rssi, float snr);
    void handleGPSPacket(const LoRaPacket* packet, float rssi, float snr);
    void handleHeartbeatPacket(const LoRaPacket* packet, float rssi, float snr);
    void handleConfigCommandPacket(const LoRaPacket* packet, float rssi, float snr);
    
    // Gestión de eventos internos
    void onSystemEvent(const char* event, const char* details = nullptr);
    void onStatusChange(LoRaStatus newStatus);
    void onMeshEventInternal(const char* event, uint16_t nodeID);
    
    // Utilidades internas
    void updateStatistics();
    void performPeriodicMaintenance();
    bool checkSystemHealth();
    void logSystemEvent(const char* event, const char* details);
    
    // Conversión de datos
    void gpsDataToPayload(float lat, float lon, uint32_t timestamp, GPSPayload* payload);
    void payloadToGpsData(const GPSPayload* payload, float* lat, float* lon, uint32_t* timestamp);
    
    // Configuración por defecto
    void setDefaultConfiguration();
    bool validateSystemConfiguration();
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en lora_manager.cpp
 */
extern LoRaManager loraManager;

/*
 * UTILIDADES HELPER PARA LA APLICACIÓN
 */

// Inicializar sistema LoRa con configuración simple
bool initializeLoRaSystem(uint16_t deviceID, DeviceRole role);

// Configuración rápida para diferentes roles
bool configureForTracker(uint16_t deviceID);
bool configureForRepeater(uint16_t deviceID);
bool configureForReceiver(uint16_t deviceID);

// Obtener información resumida del sistema
String getSystemSummary();

// Verificar compatibilidad de versiones entre nodos
bool checkVersionCompatibility(const String& remoteVersion);

/*
 * CONSTANTES DE CONFIGURACIÓN POR DEFECTO
 */

// Configuraciones predefinidas por role
extern const LoRaManagerConfig DEFAULT_TRACKER_CONFIG;
extern const LoRaManagerConfig DEFAULT_REPEATER_CONFIG;
extern const LoRaManagerConfig DEFAULT_RECEIVER_CONFIG;

// Timeouts y intervalos por defecto
#define DEFAULT_PACKET_TIMEOUT      10000   // 10 segundos
#define DEFAULT_STATS_INTERVAL      30000   // 30 segundos
#define DEFAULT_MAINTENANCE_INTERVAL 60000   // 1 minuto
#define DEFAULT_HEALTH_CHECK_INTERVAL 15000  // 15 segundos

#endif // LORA_MANAGER_H