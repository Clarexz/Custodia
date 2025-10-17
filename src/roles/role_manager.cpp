/*
 * ROLE_MANAGER.CPP - Coordinador de Roles del Sistema
 */

#include "role_manager.h"
#include "tracker_role.h"
#include "repeater_role.h"
#include "receiver_role.h"
#include "end_node_repeater_role.h"
#include "../config/config_manager.h"
#include "../gps/gps_manager.h"
#include "../lora.h"

// Instancia global
RoleManager roleManager;

/*
 * CONSTRUCTOR
 */
RoleManager::RoleManager() {
    loraInitialized = false;
}

/*
 * DESTRUCTOR
 */
RoleManager::~RoleManager() {
    // Cleanup si es necesario
}

/*
 * INICIALIZACIÓN PARA ROL ACTUAL
 */
void RoleManager::initializeForRole() {
    if (configManager.isConfigValid()) {
        initializeGPSForRole();
        initializeLoRaForRole();
        loraInitialized = true;
    }
}

/*
 * MANEJO DE MODO OPERATIVO PRINCIPAL
 */
void RoleManager::handleOperativeMode() {
    DeviceConfig config = configManager.getConfig();
    
    // Verificación adicional de que LoRa esté funcionando
    if (loraManager.getStatus() == LORA_STATUS_ERROR) {
        Serial.println("[MAIN] ERROR: LoRa en estado de error. Reintentando inicialización...");
        initializeLoRaForRole();
        delay(2000);
        return;
    }
    
    switch (config.role) {
        case ROLE_TRACKER:
            trackerRole.handleMode();
            break;
            
        case ROLE_REPEATER:
            repeaterRole.handleMode();
            break;
            
        case ROLE_RECEIVER:
            receiverRole.handleMode();
            break;
        case ROLE_END_NODE_REPEATER:
            endNodeRepeaterRole.handleMode();
            break;
            
        default:
            configManager.setState(STATE_CONFIG_MODE);
            loraInitialized = false;
            Serial.println("[ERROR] Rol no válido. Entrando en modo configuración.");
            break;
    }
}

/*
 * INICIALIZACIÓN DE LORA SEGÚN ROL
 */
void RoleManager::initializeLoRaForRole() {
    DeviceConfig config = configManager.getConfig();
    
    //Serial.println("[MAIN] Inicializando LoRa para rol: " + String(config.role));
    
    // Inicializar LoRa con el device ID configurado
    if (!loraManager.begin(config.deviceID)) {
        Serial.println("[MAIN] ERROR: Fallo en inicialización LoRa");
        loraInitialized = false;
        return;
    }
    
    // Configurar role en LoRaManager para mesh priority
    loraManager.setRole(config.role);
    
    // Configuración específica según rol
    switch (config.role) {
        case ROLE_TRACKER:
            //Serial.println("[MAIN] LoRa configurado para TRACKER (CLIENT priority)");
            break;
            
        case ROLE_REPEATER:
            //Serial.println("[MAIN] LoRa configurado para REPEATER (ROUTER priority)");
            break;
            
        case ROLE_RECEIVER:
            //Serial.println("[MAIN] LoRa configurado para RECEIVER (CLIENT priority + Remote Config)");
            break;
        case ROLE_END_NODE_REPEATER:
            //Serial.println("[MAIN] LoRa configurado para END_NODE_REPEATER (store-and-forward)");
            break;
            
        default:
            //Serial.println("[MAIN] Rol no reconocido - LoRa en configuración por defecto");
            break;
    }
    
    // Mostrar configuración solo en modo ADMIN
    /*if (configManager.isAdminMode()) {
        loraManager.printConfiguration();
        
        Serial.println("[MAIN] === CONFIGURACIÓN MESH ===");
        Serial.println("Algoritmo: Meshtastic Managed Flood Routing");
        Serial.println("Max hops: " + String(MESHTASTIC_MAX_HOPS));
        Serial.println("Duplicate detection: ACTIVO");
        Serial.println("SNR-based delays: ACTIVO");
        Serial.println("Role priority: " + String(config.role == ROLE_REPEATER ? "ALTA" : "NORMAL"));
        if (config.role == ROLE_RECEIVER) {
            Serial.println("Remote config: ACTIVO");
        }
        Serial.println("=============================");
    }
    */
}

/*
 * INICIALIZACIÓN DE GPS SEGÚN ROL
 */
void RoleManager::initializeGPSForRole() {
    DeviceConfig config = configManager.getConfig();
    
    //Serial.println("[MAIN] Inicializando GPS para rol: " + String(config.role));
    
    switch (config.role) {
        case ROLE_TRACKER:
            gpsManager.begin();
            gpsManager.setUpdateInterval(config.gpsInterval * 1000U);
            break;
        case ROLE_REPEATER:
            gpsManager.begin();
            break;
        case ROLE_RECEIVER:
            gpsManager.begin();
            break;
        case ROLE_END_NODE_REPEATER:
            // No inicializar GPS; los pines se reutilizan para UART con el gateway
            Serial.println("[MAIN] Rol END_NODE_REPEATER: GPS deshabilitado (pines reservados para UART).");
            break;
            
        default:
            Serial.println("[MAIN] Rol no reconocido - GPS en modo fijo");
            gpsManager.begin();
            break;
    }
}
