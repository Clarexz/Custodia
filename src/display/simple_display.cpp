/*
 * SIMPLE_DISPLAY.CPP - Modo de visualización SIMPLE
 */

#include "simple_display.h"

// Instancia global
SimpleDisplay simpleDisplay;

/*
 * CONSTRUCTOR
 */
SimpleDisplay::SimpleDisplay() {
    // Inicialización si es necesaria
}

/*
 * DESTRUCTOR
 */
SimpleDisplay::~SimpleDisplay() {
    // Cleanup si es necesario
}

/*
 * MOSTRAR OUTPUT SIMPLE DEL TRACKER
 */
void SimpleDisplay::showTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent) {
    // Formato simple según key requirements: [deviceID, latitude, longitude, batteryvoltage, timestamp]
    String packet = String(deviceID) + "," + 
                   String(lat, 6) + "," + 
                   String(lon, 6) + "," + 
                   String(battery) + "," + 
                   String(timestamp);
    
    Serial.println("[" + packet + "]");
    if (sent) {
        Serial.println("Envío realizado");
    } else {
        Serial.println("Error en envío");
    }
    Serial.println();
}

/*
 * MOSTRAR OUTPUT SIMPLE DEL REPEATER
 */
void SimpleDisplay::showRepeaterOutput(const String& packet) {
    // Mostrar solo el packet que se está retransmitiendo
    Serial.println("[" + packet + "]");
    Serial.println("Retransmisión realizada");
    Serial.println();
}

/*
 * MOSTRAR OUTPUT SIMPLE DEL RECEIVER
 */
void SimpleDisplay::showReceiverOutput(const String& packet) {
    // Mostrar solo el packet recibido
    Serial.println("[" + packet + "]");
    Serial.println("Datos recibidos");
    Serial.println();
}