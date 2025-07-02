/*
 * REPEATER_ROLE.H - Lógica específica del rol REPEATER
 */

#ifndef REPEATER_ROLE_H
#define REPEATER_ROLE_H

#include <Arduino.h>

/*
 * CLASE PARA MANEJO DEL ROL REPEATER
 */
class RepeaterRole {
private:
    // Variables de estado para REPEATER
    unsigned long lastActivity;
    unsigned long lastStatusCheck;
    uint32_t lastRebroadcastCount;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    RepeaterRole();
    ~RepeaterRole();
    
    /*
     * MÉTODO PRINCIPAL
     */
    
    // Ejecutar lógica del REPEATER
    void handleMode();
};

/*
 * INSTANCIA GLOBAL
 */
extern RepeaterRole repeaterRole;

#endif