/*
 * CONFIG.H - Sistema de Configuración para Custom Meshtastic GPS Tracker
 * 
 * Este archivo define la estructura del sistema de configuración que permite
 * configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa.
 * 
 * Características principales:
 * - Configuración via comandos seriales
 * - Persistencia en EEPROM usando Preferences
 * - Validación automática de parámetros
 * - Sistema de estados para control de flujo
 * - ACTUALIZADO: Modo de visualización de datos (SIMPLE/ADMIN) con comandos en tiempo real
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

/*
 * ENUMERACIONES
 */

// Roles disponibles para cada dispositivo en la red mesh
enum DeviceRole {
    ROLE_NONE = 0,      // Sin configurar (estado inicial)
    ROLE_TRACKER = 1,   // Dispositivo que transmite su posición GPS
    ROLE_REPEATER = 2,  // Dispositivo que solo retransmite mensajes
    ROLE_RECEIVER = 3   // Dispositivo que recibe y muestra posiciones
};

// Estados operativos del sistema
enum SystemState {
    STATE_BOOT = 0,         // Inicializando sistema
    STATE_CONFIG_MODE = 1,  // Esperando comandos de configuración
    STATE_RUNNING = 2,      // Ejecutando función según rol asignado
    STATE_SLEEP = 3         // Modo de bajo consumo (futuro)
};

// Modos de visualización de datos
enum DataDisplayMode {
    DATA_MODE_SIMPLE = 0,   // Vista simple: solo packet básico
    DATA_MODE_ADMIN = 1     // Vista admin: información completa
};

/*
 * ESTRUCTURAS DE DATOS
 */

// Estructura que contiene toda la configuración del dispositivo
struct DeviceConfig {
    DeviceRole role;         // Rol asignado al dispositivo
    uint16_t deviceID;       // ID único del dispositivo (1-999)
    uint16_t gpsInterval;    // Segundos entre transmisiones GPS (5-3600)
    uint8_t maxHops;         // Máximo número de saltos en mesh (1-10)
    DataDisplayMode dataMode; // Modo de visualización de datos
    bool configValid;        // Flag que indica si la configuración es válida
    char version[8];         // Versión del firmware para compatibilidad
};

/*
 * CLASE PRINCIPAL - ConfigManager
 * 
 * Esta clase maneja todo el sistema de configuración del dispositivo.
 * Proporciona una interfaz de comandos seriales para configurar el rol,
 * parámetros de red y comportamiento del dispositivo.
 */
class ConfigManager {
private:
    // Instancia de Preferences para manejo de EEPROM
    Preferences preferences;
    
    // Configuración actual del dispositivo
    DeviceConfig config;
    
    // Estado actual del sistema
    SystemState currentState;
    
    /*
     * MÉTODOS PRIVADOS - Solo usados internamente
     */
    
    // Establece configuración por defecto
    void setDefaultConfig();
    
    // Muestra el prompt del sistema de configuración
    void printPrompt();
    
    // Convierte enum DeviceRole a string legible
    String getRoleString(DeviceRole role);
    
    // Convierte enum SystemState a string legible
    String getStateString(SystemState state);
    
    // Convierte enum DataDisplayMode a string legible
    String getDataModeString(DataDisplayMode mode);
    
public:
    /*
     * CONSTRUCTOR
     */
    ConfigManager();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    
    // Inicializa el sistema de configuración
    void begin();
    
    // Procesa comandos recibidos por puerto serial
    void processSerialInput();
    
    // Guarda configuración actual en EEPROM
    void saveConfig();
    
    // Carga configuración desde EEPROM
    void loadConfig();
    
    // Resetea configuración a valores por defecto
    void resetToDefaults();
    
    /*
     * MANEJADORES DE COMANDOS ESPECÍFICOS
     */
    
    // CONFIG_ROLE: Configura el rol del dispositivo
    void handleConfigRole(String value);
    
    // CONFIG_DEVICE_ID: Establece ID único del dispositivo
    void handleConfigDeviceID(String value);
    
    // CONFIG_GPS_INTERVAL: Configura intervalo de transmisión GPS
    void handleConfigGpsInterval(String value);
    
    // CONFIG_MAX_HOPS: Establece máximo de saltos en mesh
    void handleConfigMaxHops(String value);
    
    // CONFIG_DATA_MODE: Configura modo de visualización de datos (ACTUALIZADO)
    void handleConfigDataMode(String value);
    
    // NUEVO: MODE: Cambia modo durante operación
    void handleModeChange(String value);
    
    // CONFIG_SAVE: Guarda configuración en EEPROM
    void handleConfigSave();
    
    // CONFIG_RESET: Resetea configuración (con confirmación)
    void handleConfigReset();
    
    // INFO: Muestra información del dispositivo
    void handleInfo();
    
    // STATUS: Muestra estado y configuración actual
    void handleStatus();
    
    // HELP: Muestra ayuda de comandos disponibles
    void handleHelp();
    
    /*
     * MÉTODOS DE ACCESO (GETTERS Y SETTERS)
     */
    
    // Obtiene copia de la configuración actual
    DeviceConfig getConfig() { return config; }
    
    // Obtiene estado actual del sistema
    SystemState getState() { return currentState; }
    
    // Verifica si la configuración es válida
    bool isConfigValid() { return config.configValid; }
    
    // Cambia el estado del sistema
    void setState(SystemState state) { currentState = state; }
    
    // Obtiene modo de visualización actual
    DataDisplayMode getDataMode() { return config.dataMode; }
    
    // Verifica si está en modo simple
    bool isSimpleMode() { return config.dataMode == DATA_MODE_SIMPLE; }
    
    // Verifica si está en modo admin
    bool isAdminMode() { return config.dataMode == DATA_MODE_ADMIN; }
    
    // NUEVO: Cambiar modo directamente (para comandos en tiempo real)
    void setDataMode(DataDisplayMode mode);
    
    // NUEVO: Obtener string del modo actual para confirmación
    String getCurrentDataModeString();
    
    /*
     * MÉTODOS UTILITARIOS
     */
    
    // Imprime configuración actual en formato legible
    void printConfig();
    
    // Muestra mensaje de bienvenida al iniciar
    void printWelcome();
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en config.cpp para uso en todo el proyecto
 */
extern ConfigManager configManager;

#endif