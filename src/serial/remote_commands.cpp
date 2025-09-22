/*
 * REMOTE_COMMANDS.CPP - Comandos Remotos para RECEIVER
 */

#include "remote_commands.h"
#include "../config/config_manager.h"
#include "../lora.h"
#include "../roles/receiver_role.h"
#include "../roles/role_manager.h" 

// Instancia global
RemoteCommands remoteCommands;

/*
 * CONSTRUCTOR
 */
RemoteCommands::RemoteCommands() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
RemoteCommands::~RemoteCommands() {
    // Cleanup si es necesario
}

/*
 * COMANDOS EN MODO NORMAL DEL RECEIVER
 */
void RemoteCommands::handleNormalModeCommands(String input) {
    if (input == "DISCOVER") {
        handleDiscoverCommand();
    }
    else if (input.startsWith("REMOTE_CONFIG ")) {
        String deviceIDStr = input.substring(14);
        deviceIDStr.trim();
        
        int devID = deviceIDStr.toInt();
        if (devID >= 1 && devID <= 999) {
            receiverRole.setState(RECEIVER_REMOTE_CONFIG);
            receiverRole.setTargetDeviceID(devID);
            Serial.println("[INFO] Configurando dispositivo " + String(devID) + "...");
            Serial.println("[INFO] Comandos: REMOTE_GPS_INTERVAL, REMOTE_DATA_MODE, REMOTE_STATUS, REMOTE_REBOOT, EXIT");
            Serial.print("remote_" + String(devID, DEC) + "> ");
        } else {
            Serial.println("[ERROR] Device ID inválido. Use un número entre 1 y 999.");
        }
    }
    else if (input.startsWith("MODE ")) {
        configManager.handleModeChange(input.substring(5));
    }
    else if (input == "STATUS") {
        configManager.handleStatus();
    }
    else if (input == "INFO") {
        configManager.handleInfo();
    }
    // AGREGAR ESTA LÍNEA:
    else if (input == "CONFIG_RESET") {
        configManager.handleConfigReset();
    }
    // AGREGAR ESTA LÍNEA:
    else if (input == "CONFIG") {
        configManager.setState(STATE_CONFIG_MODE);
        roleManager.setLoRaInitialized(false);
        Serial.println("[INFO] Entrando en modo configuración.");
    }
    else if (input == "HELP") {
        Serial.println("\n=== COMANDOS RECEIVER ===");
        Serial.println("DISCOVER                     - Buscar dispositivos en red");
        Serial.println("REMOTE_CONFIG <deviceID>     - Configurar dispositivo remoto");
        Serial.println("MODE SIMPLE/ADMIN            - Cambiar modo visualización");
        Serial.println("STATUS/INFO                  - Información del sistema");
        Serial.println("============================");
    }
    else {
        Serial.println("[ERROR] Comando no reconocido. Use HELP para ver comandos.");
    }
}

/*
 * COMANDOS EN MODO CONFIGURACIÓN REMOTA
 */
void RemoteCommands::handleRemoteConfigCommands(String input) {
    if (input == "EXIT") {
        receiverRole.setState(RECEIVER_NORMAL);
        receiverRole.setTargetDeviceID(0);
        Serial.println("[INFO] Saliendo de configuración remota");
        Serial.println("[RECEIVER] Volviendo a modo normal...");
        return;
    }
    
    if (input.startsWith("REMOTE_GPS_INTERVAL ")) {
        String valueStr = input.substring(20);
        int value = valueStr.toInt();
        
        if (value >= 5 && value <= 3600) {
            sendRemoteConfigCommand(REMOTE_CMD_GPS_INTERVAL, value);
        } else {
            Serial.println("[ERROR] Intervalo inválido. Use 5-3600 segundos.");
        }
    }
    else if (input.startsWith("REMOTE_DATA_MODE ")) {
        String modeStr = input.substring(17);
        modeStr.trim();
        
        if (modeStr == "SIMPLE") {
            sendRemoteConfigCommand(REMOTE_CMD_DATA_MODE, 0);
        } else if (modeStr == "ADMIN") {
            sendRemoteConfigCommand(REMOTE_CMD_DATA_MODE, 1);
        } else {
            Serial.println("[ERROR] Modo inválido. Use SIMPLE o ADMIN.");
        }
    }
    else if (input == "REMOTE_STATUS") {
        sendRemoteConfigCommand(REMOTE_CMD_STATUS, 0);
    }
    else if (input == "REMOTE_REBOOT") {
        Serial.print("[WARNING] ¿Reiniciar device " + String(receiverRole.getTargetDeviceID()) + "? (Y/N): ");
        
        // Esperar confirmación
        unsigned long startTime = millis();
        while (millis() - startTime < 10000) {
            if (Serial.available()) {
                String confirm = Serial.readStringUntil('\n');
                confirm.trim();
                confirm.toUpperCase();
                Serial.println(confirm);
                
                if (confirm == "Y" || confirm == "YES") {
                    sendRemoteConfigCommand(REMOTE_CMD_REBOOT, 0);
                    return;
                } else {
                    Serial.println("[INFO] Reboot cancelado.");
                    Serial.print("remote_" + String(receiverRole.getTargetDeviceID(), DEC) + "> ");
                    return;
                }
            }
            delay(100);
        }
        Serial.println("\n[INFO] Timeout. Reboot cancelado.");
    }
    else if (input == "HELP") {
        Serial.println("\n=== COMANDOS CONFIGURACIÓN REMOTA ===");
        Serial.println("REMOTE_GPS_INTERVAL <5-3600>    - Cambiar intervalo GPS");
        Serial.println("REMOTE_DATA_MODE <SIMPLE|ADMIN> - Cambiar modo datos");
        Serial.println("REMOTE_STATUS                   - Obtener estado");
        Serial.println("REMOTE_REBOOT                   - Reiniciar dispositivo");
        Serial.println("EXIT                            - Salir configuración remota");
        Serial.println("=====================================");
    }
    else {
        Serial.println("[ERROR] Comando no reconocido. Use HELP para ver comandos.");
    }
    
    // Mostrar prompt
    Serial.print("remote_" + String(receiverRole.getTargetDeviceID(), DEC) + "> ");
}

/*
 * COMANDO DISCOVER
 */
void RemoteCommands::handleDiscoverCommand() {
    Serial.println("[INFO] Buscando dispositivos en la red...");
    
    if (loraManager.sendDiscoveryRequest()) {
        Serial.println("[INFO] Discovery request enviado. Esperando respuestas...");
        
        // Esperar respuestas por 3 segundos
        unsigned long startTime = millis();
        
        while (millis() - startTime < DISCOVERY_TIMEOUT) {
            processIncomingMessages();
            delay(100);
        }
        
        Serial.println("[INFO] Discovery completado.");
    } else {
        Serial.println("[ERROR] No se pudo enviar discovery request");
    }
}

/*
 * ENVIAR COMANDO REMOTO
 */
void RemoteCommands::sendRemoteConfigCommand(uint8_t cmdType, uint32_t value) {
    Serial.println("[OK] Enviando comando a device " + String(receiverRole.getTargetDeviceID()) + "...");
    
    if (loraManager.sendRemoteConfigCommand(receiverRole.getTargetDeviceID(), (RemoteCommandType)cmdType, value, receiverRole.getNextCommandSequence())) {
        Serial.println("[INFO] Comando enviado. Esperando respuesta...");
    } else {
        Serial.println("[ERROR] Fallo al enviar comando");
    }
}

/*
 * PROCESAR MENSAJES ENTRANTES
 */
void RemoteCommands::processIncomingMessages() {
    if (loraManager.isPacketAvailable()) {
        LoRaPacket packet;
        if (loraManager.receivePacket(&packet)) {
            
            // Procesar según tipo de mensaje
            switch (packet.messageType) {
                case MSG_GPS_DATA:
                    // Procesar datos GPS (existente)
                    break;
                    
                case MSG_DISCOVERY_REQUEST:
                    loraManager.processDiscoveryRequest(&packet);
                    break;
                    
                case MSG_DISCOVERY_RESPONSE:
                    loraManager.processDiscoveryResponse(&packet);
                    break;
                    
                case MSG_CONFIG_CMD:
                    loraManager.processRemoteConfigCommand(&packet);
                    break;
                    
                case MSG_CONFIG_RESPONSE:
                    loraManager.processRemoteConfigResponse(&packet);
                    break;
                    
                default:
                    break;
            }
        }
    }
}
