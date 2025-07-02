/*
 * TRACKER_ROLE.H - Lógica específica del rol TRACKER
 */

#ifndef TRACKER_ROLE_H
#define TRACKER_ROLE_H

#include <Arduino.h>

/*
 * CLASE PARA MANEJO DEL ROL TRACKER
 */
class TrackerRole {
private:
    // Variables de estado para TRACKER
    unsigned long lastGPSTransmission;
    unsigned long lastStatusCheck;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    TrackerRole();
    ~TrackerRole();
    
    /*
     * MÉTODO PRINCIPAL
     */
    
    // Ejecutar lógica del TRACKER
    void handleMode();
};

/*
 * INSTANCIA GLOBAL
 */
extern TrackerRole trackerRole;

#endif