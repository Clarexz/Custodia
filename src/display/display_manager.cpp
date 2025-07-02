/*
 * DISPLAY_MANAGER.CPP - Coordinador de Visualización
 */

#include "display_manager.h"
#include "simple_display.h"
#include "admin_display.h"
#include "../config/config_manager.h"

// Instancia global
DisplayManager displayManager;

/*
 * CONSTRUCTOR
 */
DisplayManager::DisplayManager() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
DisplayManager::~DisplayManager() {
    // Cleanup si es necesario
}

/*
 * MOSTRAR OUTPUT DEL TRACKER
 */
void DisplayManager::showTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent) {
    if (configManager.isSimpleMode()) {
        simpleDisplay.showTrackerOutput(deviceID, lat, lon, battery, timestamp, sent);
    } else {
        adminDisplay.showTrackerOutput(deviceID, sent);
    }
}

/*
 * MOSTRAR OUTPUT DEL REPEATER
 */
void DisplayManager::showSimpleRepeaterOutput(const String& packet) {
    simpleDisplay.showRepeaterOutput(packet);
}

void DisplayManager::showAdminRepeaterOutput() {
    adminDisplay.showRepeaterOutput();
}

/*
 * MOSTRAR OUTPUT DEL RECEIVER
 */
void DisplayManager::showSimpleReceiverOutput(const String& packet) {
    simpleDisplay.showReceiverOutput(packet);
}

void DisplayManager::showAdminReceiverOutput() {
    adminDisplay.showReceiverOutput();
}