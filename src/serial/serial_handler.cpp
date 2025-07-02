/*
 * SERIAL_HANDLER.CPP - Manejador de Comandos Seriales
 */

#include "serial_handler.h"
#include "remote_commands.h"
#include "../config/config_manager.h"
#include "../roles/role_manager.h"
#include "../roles/receiver_role.h"

// Instancia global
SerialHandler serialHandler;

/*
 * CONSTRUCTOR
 */
SerialHandler::SerialHandler() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
SerialHandler::~SerialHandler() {
    // Cleanup si es necesario
}

/*
 * PROCESADOR PRINCIPAL DE COMANDOS SERIALES
 */
void SerialHandler::processSerialInput() {
    if (!Serial.available()) return;
    
    // Procesar comandos seriales según el rol y estado
    if (configManager.getState() == STATE_RUNNING && 
        configManager.getConfig().role == ROLE_RECEIVER) {
        // RECEIVER tiene manejo especial de comandos
        handleReceiverSerialInput();
    } else if (configManager.getState() == STATE_RUNNING) {
        // Otros roles - comandos limitados durante operación
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();
        
        if (input.length() > 0) {
            handleOperationCommands(input);
        }
    } else {
        // Modo configuración normal
        configManager.processSerialInput();
    }
}

/*
 * COMANDOS LIMITADOS DURANTE OPERACIÓN
 */
void SerialHandler::handleOperationCommands(String input) {
    Serial.println(">" + input);
    
    if (input.startsWith("MODE ")) {
        configManager.handleModeChange(input.substring(5));
    } else if (input == "CONFIG_RESET") {
        configManager.handleConfigReset();
    } else if (input == "CONFIG") {
        configManager.setState(STATE_CONFIG_MODE);
        roleManager.setLoRaInitialized(false);
        Serial.println("[INFO] Entrando en modo configuración.");
    } else if (input == "STATUS") {
        configManager.handleStatus();
    } else if (input == "INFO") {
        configManager.handleInfo();
    } else if (input == "HELP") {
        Serial.println("\n=== COMANDOS DURANTE OPERACIÓN ===");
        Serial.println("MODE SIMPLE/ADMIN    - Cambiar modo visualización");
        Serial.println("CONFIG_RESET         - Resetear configuración");
        Serial.println("CONFIG               - Modo configuración");
        Serial.println("STATUS/INFO/HELP     - Información");
        Serial.println("============================");
    } else {
        Serial.println("[INFO] Comandos limitados en operación. Use HELP para ver disponibles.");
    }
}

/*
 * MANEJO ESPECIAL PARA RECEIVER
 */
void SerialHandler::handleReceiverSerialInput() {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    
    if (input.length() == 0) {
        if (receiverRole.getState() == RECEIVER_REMOTE_CONFIG) {
            Serial.print("remote_" + String(receiverRole.getTargetDeviceID(), DEC) + "> ");
        }
        return;
    }
    
    // Echo del comando
    Serial.println(">" + input);
    
    if (receiverRole.getState() == RECEIVER_NORMAL) {
        remoteCommands.handleNormalModeCommands(input);
    } else if (receiverRole.getState() == RECEIVER_REMOTE_CONFIG) {
        remoteCommands.handleRemoteConfigCommands(input);
    }
}