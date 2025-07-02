/*
 * ROLE_MANAGER.H - Coordinador de Roles del Sistema
 * 
 * Coordina la ejecución de lógica específica según el rol configurado
 */

#ifndef ROLE_MANAGER_H
#define ROLE_MANAGER_H

#include <Arduino.h>
#include "../config/config_manager.h"

/*
 * CLASE COORDINADORA DE ROLES
 */
class RoleManager {
private:
    // Variables para controle de inicialización
    bool loraInitialized;
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    RoleManager();
    ~RoleManager();
    
    /*
     * MÉTODOS PRINCIPALES
     */
    
    // Inicializar sistemas según rol
    void initializeForRole();
    
    // Ejecutar lógica según rol actual
    void handleOperativeMode();
    
    // Control de inicialización de LoRa
    bool isLoRaInitialized() { return loraInitialized; }
    void setLoRaInitialized(bool state) { loraInitialized = state; }
    
    /*
     * MÉTODOS DE INICIALIZACIÓN
     */
    
    // Inicializar LoRa según rol
    void initializeLoRaForRole();
    
    // Inicializar GPS según rol
    void initializeGPSForRole();
};

/*
 * INSTANCIA GLOBAL
 */
extern RoleManager roleManager;

#endif