/*
 * REPEATER_ROLE.CPP - Lógica específica del rol REPEATER
 */

#include "repeater_role.h"
#include "../config/config_manager.h"
#include "../lora.h"
#include "../display/display_manager.h"
#include "user_logic.h"

// Instancia global
RepeaterRole repeaterRole;

/*
 * CONSTRUCTOR
 */
RepeaterRole::RepeaterRole() {
    lastActivity = 0;
    lastStatusCheck = 0;
    lastRebroadcastCount = 0;
}

/*
 * DESTRUCTOR
 */
RepeaterRole::~RepeaterRole() {
    // Cleanup si es necesario
}

/*
 * LÓGICA PRINCIPAL DEL REPEATER
 */
void RepeaterRole::handleMode() {
    unsigned long currentTime = millis();
    
    // Verificación periódica del estado de LoRa
    if (currentTime - lastStatusCheck >= 30000) {
        lastStatusCheck = currentTime;
        if (loraManager.getStatus() == LORA_STATUS_ERROR) {
            Serial.println("[REPEATER] ERROR: LoRa en estado de error. Reinicializando...");
            // initializeLoRaForRole(); // Se manejará desde RoleManager
            return;
        }
    }
    
    // LED parpadeando muy rápido para indicar actividad de repetidor
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    
    // Verificar si hubo nuevas retransmisiones para modo SIMPLE
    LoRaStats stats = loraManager.getStats();
    if (configManager.isSimpleMode() && stats.rebroadcasts > lastRebroadcastCount) {
        // Generar packet simulado para mostrar en modo simple
        String packet = "002,25.302677,-98.277664,3950,1718661234"; // Ejemplo
        displayManager.showSimpleRepeaterOutput(packet);
        lastRebroadcastCount = stats.rebroadcasts;
    }
    
    // Mostrar estado según intervalo y modo
    if (currentTime - lastActivity >= 10000) {
        lastActivity = currentTime;
        
        if (configManager.isAdminMode()) {
            displayManager.showAdminRepeaterOutput();
        }
    }
    
    delay(100);
}