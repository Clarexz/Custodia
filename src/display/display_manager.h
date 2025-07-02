/*
 * DISPLAY_MANAGER.H - Coordinador de Visualización
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

/*
 * CLASE COORDINADORA DE DISPLAY
 */
class DisplayManager {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    DisplayManager();
    ~DisplayManager();
    
    /*
     * MÉTODOS PARA TRACKER
     */
    void showTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent);
    
    /*
     * MÉTODOS PARA REPEATER
     */
    void showSimpleRepeaterOutput(const String& packet);
    void showAdminRepeaterOutput();
    
    /*
     * MÉTODOS PARA RECEIVER
     */
    void showSimpleReceiverOutput(const String& packet);
    void showAdminReceiverOutput();
};

/*
 * INSTANCIA GLOBAL
 */
extern DisplayManager displayManager;

#endif