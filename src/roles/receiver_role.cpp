/*
 * RECEIVER_ROLE.CPP - Lógica específica del rol RECEIVER
 */

#include "receiver_role.h"
#include "../config/config_manager.h"
#include "../gps/gps_manager.h"
#include "../lora.h"
#include "../display/display_manager.h"
#include "user_logic.h"

// Instancia global
ReceiverRole receiverRole;

/*
 * CONSTRUCTOR
 */
ReceiverRole::ReceiverRole() {
    lastStatusUpdate = 0;
    lastPacketCount = 0;
    lastStatusCheck = 0;
    receiverState = RECEIVER_NORMAL;
    targetDeviceID = 0;
    commandSequence = 1;
}

/*
 * DESTRUCTOR
 */
ReceiverRole::~ReceiverRole() {
    // Cleanup si es necesario
}

/*
 * LÓGICA PRINCIPAL DEL RECEIVER
 */
void ReceiverRole::handleMode() {
    unsigned long currentTime = millis();
    
    // Verificación periódica del estado de LoRa
    if (currentTime - lastStatusCheck >= 30000) {
        lastStatusCheck = currentTime;
        if (loraManager.getStatus() == LORA_STATUS_ERROR) {
            Serial.println("[RECEIVER] ERROR: LoRa en estado de error. Reinicializando...");
            // initializeLoRaForRole(); // Se manejará desde RoleManager
            return;
        }
    }
    
    // LED encendido constante para indicar recepción activa
    digitalWrite(LED_PIN, HIGH);
    
    // Verificar si se recibieron nuevos packets
    LoRaStats stats = loraManager.getStats();
    if (stats.packetsReceived > lastPacketCount) {
        if (configManager.isSimpleMode()) {
            String packet;
            if (loraManager.fetchSimplePacket(packet)) {
                displayManager.showSimpleReceiverOutput(packet);
            }
        }
        lastPacketCount = stats.packetsReceived;
    }
    
    // Mostrar estado cada 5 segundos solo en modo ADMIN
    if (currentTime - lastStatusUpdate >= 5000) {
        lastStatusUpdate = currentTime;
        
        if (configManager.isAdminMode() && receiverState == RECEIVER_NORMAL) {
            displayManager.showAdminReceiverOutput();
        }
    }
    
    delay(500);
}
