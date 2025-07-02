/*
 * SIMPLE_DISPLAY.H - Modo de visualización SIMPLE
 */

#ifndef SIMPLE_DISPLAY_H
#define SIMPLE_DISPLAY_H

#include <Arduino.h>

/*
 * CLASE PARA DISPLAY SIMPLE
 */
class SimpleDisplay {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    SimpleDisplay();
    ~SimpleDisplay();
    
    /*
     * MÉTODOS DE DISPLAY SIMPLE
     */
    void showTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent);
    void showRepeaterOutput(const String& packet);
    void showReceiverOutput(const String& packet);
};

/*
 * INSTANCIA GLOBAL
 */
extern SimpleDisplay simpleDisplay;

#endif