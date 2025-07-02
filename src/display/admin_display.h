/*
 * ADMIN_DISPLAY.H - Modo de visualización ADMIN
 */

#ifndef ADMIN_DISPLAY_H
#define ADMIN_DISPLAY_H

#include <Arduino.h>

/*
 * CLASE PARA DISPLAY ADMIN
 */
class AdminDisplay {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    AdminDisplay();
    ~AdminDisplay();
    
    /*
     * MÉTODOS DE DISPLAY ADMIN
     */
    void showTrackerOutput(uint16_t deviceID, bool sent);
    void showRepeaterOutput();
    void showReceiverOutput();
};

/*
 * INSTANCIA GLOBAL
 */
extern AdminDisplay adminDisplay;

#endif