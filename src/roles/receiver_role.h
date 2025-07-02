/*
 * RECEIVER_ROLE.H - Lógica específica del rol RECEIVER
 */

#ifndef RECEIVER_ROLE_H
#define RECEIVER_ROLE_H

#include <Arduino.h>

/*
 * ESTADOS ESPECÍFICOS DEL RECEIVER
 */
enum ReceiverState {
    RECEIVER_NORMAL = 0,        // Modo normal escuchando
    RECEIVER_REMOTE_CONFIG = 1  // Configurando dispositivo remoto
};

/*
 * CLASE PARA MANEJO DEL ROL RECEIVER
 */
class ReceiverRole {
private:
    // Variables de estado para RECEIVER
    unsigned long lastStatusUpdate;
    uint32_t lastPacketCount;
    unsigned long lastStatusCheck;
    
    // Variables para configuración remota
    ReceiverState receiverState;
    uint16_t targetDeviceID;
    uint32_t commandSequence;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    ReceiverRole();
    ~ReceiverRole();
    
    /*
     * MÉTODO PRINCIPAL
     */
    
    // Ejecutar lógica del RECEIVER
    void handleMode();
    
    /*
     * GETTERS Y SETTERS PARA ESTADO
     */
    ReceiverState getState() { return receiverState; }
    void setState(ReceiverState state) { receiverState = state; }
    uint16_t getTargetDeviceID() { return targetDeviceID; }
    void setTargetDeviceID(uint16_t id) { targetDeviceID = id; }
    uint32_t getNextCommandSequence() { return ++commandSequence; }
};

/*
 * INSTANCIA GLOBAL
 */
extern ReceiverRole receiverRole;

#endif