/*
 * REMOTE_COMMANDS.H - Comandos Remotos para RECEIVER
 */

#ifndef REMOTE_COMMANDS_H
#define REMOTE_COMMANDS_H

#include <Arduino.h>

/*
 * CLASE PARA COMANDOS REMOTOS
 */
class RemoteCommands {
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    RemoteCommands();
    ~RemoteCommands();
    
    /*
     * MÉTODOS PRINCIPALES
     */
    
    // Comandos en modo normal del RECEIVER
    void handleNormalModeCommands(String input);
    
    // Comandos en modo configuración remota
    void handleRemoteConfigCommands(String input);
    
    /*
     * MÉTODOS ESPECÍFICOS
     */
    
    // Comando DISCOVER
    void handleDiscoverCommand();
    
    // Enviar comando remoto
    void sendRemoteConfigCommand(uint8_t cmdType, uint32_t value);
    
    // Procesar mensajes entrantes
    void processIncomingMessages();
};

/*
 * INSTANCIA GLOBAL
 */
extern RemoteCommands remoteCommands;

#endif