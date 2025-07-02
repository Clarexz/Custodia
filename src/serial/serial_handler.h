/*
 * SERIAL_HANDLER.H - Manejador de Comandos Seriales
 */

#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <Arduino.h>

/*
 * CLASE MANEJADORA DE COMANDOS SERIALES
 */
class SerialHandler {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    SerialHandler();
    ~SerialHandler();
    
    /*
     * MÉTODOS PRINCIPALES
     */
    
    // Procesar comandos seriales según estado y rol
    void processSerialInput();
    
    /*
     * MÉTODOS ESPECÍFICOS
     */
    
    // Manejo especial para RECEIVER
    void handleReceiverSerialInput();
    
    // Comandos limitados durante operación
    void handleOperationCommands(String input);
};

/*
 * INSTANCIA GLOBAL
 */
extern SerialHandler serialHandler;

#endif