/*
 * CONFIG_MANAGER.H - Sistema de Configuración Modular
 * 
 * MODULARIZADO: Separación entre core y comandos
 * Declaraciones centralizadas para todo el sistema de configuración
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "../radio/radio_profiles.h"  // NUEVO: Para Radio Profiles

/*
 * ENUMERACIONES LOCALES
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

// Regiones LoRa con frecuencias específicas
enum LoRaRegion {
    REGION_US = 0,      // Estados Unidos/México: 915 MHz
    REGION_EU = 1,      // Europa: 868 MHz
    REGION_CH = 2,      // China: 470 MHz
    REGION_AS = 3,      // Asia: 433 MHz
    REGION_JP = 4       // Japón: 920 MHz
};

/*
 * CONSTANTES DE FRECUENCIAS POR REGIÓN
 */
#define FREQ_US_MHZ     915.0f  // Estados Unidos/México
#define FREQ_EU_MHZ     868.0f  // Europa
#define FREQ_CH_MHZ     470.0f  // China
#define FREQ_AS_MHZ     433.0f  // Asia
#define FREQ_JP_MHZ     920.0f  // Japón

// Constantes para EEPROM storage de networks
#define NETWORK_COUNT_KEY      "net_count"
#define ACTIVE_NETWORK_KEY     "active_net"
#define NETWORK_NAME_PREFIX    "net_name_"    // net_name_0, net_name_1, etc.
#define NETWORK_PASS_PREFIX    "net_pass_"    // net_pass_0, net_pass_1, etc.
#define NETWORK_HASH_PREFIX    "net_hash_"    // net_hash_0, net_hash_1, etc.
#define MAX_NETWORKS           10             // Máximo networks por dispositivo

// Estructura simple para networks
struct SimpleNetwork {
    String name;        // Convertido a uppercase automáticamente
    String password;    // Convertido a uppercase, 8-32 chars
    uint32_t hash;      // hash(name + password)
    bool active;        // true si es la network activa
    
    // Constructor por defecto
    SimpleNetwork() : hash(0), active(false) {}
    
    // Constructor con parámetros
    SimpleNetwork(String n, String p) : name(n), password(p), active(false) {
        name.toUpperCase();
        password.toUpperCase();
        hash = generateHash();
    }
    
    // Generar hash de la network
    uint32_t generateHash() {
        String combined = name + password;
        uint32_t h = 0;
        for (int i = 0; i < combined.length(); i++) {
            h = h * 31 + combined.charAt(i);
        }
        return h;
    }
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
    LoRaRegion region;       // Región LoRa para frecuencia
    RadioProfile radioProfile; // NUEVO: Perfil LoRa actual
    bool configValid;        // Flag que indica si la configuración es válida
    char version[8];         // Versión del firmware para compatibilidad
};

/*
 * CLASE PRINCIPAL - ConfigManager
 */
class ConfigManager {
private:
    // Instancia de Preferences para manejo de EEPROM
    Preferences preferences;
    
    // Configuración actual del dispositivo
    DeviceConfig config;
    
    // Estado actual del sistema
    SystemState currentState;

    // ===== NUEVAS VARIABLES PARA NETWORKS =====
    // Lista de networks disponibles
    SimpleNetwork networks[MAX_NETWORKS];
    
    // Número actual de networks guardadas
    uint8_t networkCount;
    
    // Índice de la network actualmente activa (-1 si ninguna)
    int8_t activeNetworkIndex;
    
    // ===== NUEVOS MÉTODOS PRIVADOS PARA NETWORKS =====
    void loadNetworks();           // Cargar networks desde EEPROM
    void saveNetworks();           // Guardar networks a EEPROM
    bool isValidNetworkName(String name);    // Validar nombre de network
    bool isValidPassword(String password);   // Validar password
    int findNetworkByName(String name);      // Buscar network por nombre
    String generateRandomPassword();         // Generar password aleatoria
    
    /*
     * MÉTODOS PRIVADOS
     */
    void setDefaultConfig();
    void printPrompt();
    void printWelcome();
    
public:
    /*
     * CONSTRUCTOR
     */
    ConfigManager();
    
    /*
     * MÉTODOS PÚBLICOS PRINCIPALES
     */
    void begin();
    void processSerialInput();
    void saveConfig();
    void loadConfig();
    void handleQuickConfig(String params);
    
    /*
     * GETTERS Y SETTERS
     */
    DeviceConfig getConfig() { return config; }
    SystemState getState() { return currentState; }
    bool isConfigValid() { return config.configValid; }
    void setState(SystemState state) { currentState = state; }
    DataDisplayMode getDataMode() { return config.dataMode; }
    bool isSimpleMode() { return config.dataMode == DATA_MODE_SIMPLE; }
    bool isAdminMode() { return config.dataMode == DATA_MODE_ADMIN; }
    LoRaRegion getRegion() { return config.region; }
    float getFrequencyMHz();
    void setGpsInterval(uint16_t interval);
    void setDataMode(DataDisplayMode mode);
    String getCurrentDataModeString();

    /*
     * ===== NUEVOS MÉTODOS PÚBLICOS PARA NETWORKS =====
     */
    
    // Obtener network activa actual
    SimpleNetwork* getActiveNetwork();
    
    // Obtener hash de la network activa (para packets LoRa)
    uint32_t getActiveNetworkHash();
    
    // Verificar si hay una network activa
    bool hasActiveNetwork();
    
    // Obtener número de networks guardadas
    uint8_t getNetworkCount() { return networkCount; }
    
    // Obtener network por índice (para listar)
    SimpleNetwork* getNetwork(uint8_t index);
    
    /*
     * MÉTODOS DE COMANDOS (manejadores)
     * Estos serán implementados en config_commands.cpp
     */
    void handleNetworkCreate(String params);
    void handleNetworkJoin(String params);
    void handleNetworkList();
    
    // NUEVOS: Radio Profiles getters/setters
    RadioProfile getRadioProfile() { return config.radioProfile; }
    void setRadioProfile(RadioProfile profile) { config.radioProfile = profile; }
    String getRadioProfileName();
    
    /*
     * MÉTODOS UTILITARIOS
     */
    void printConfig();
    
    /*
     * MÉTODOS DE COMANDOS
     */
    void handleConfigRole(String value);
    void handleConfigDeviceID(String value);
    void handleConfigGpsInterval(String value);
    void handleConfigMaxHops(String value);
    void handleConfigDataMode(String value);
    void handleConfigRegion(String value);
    void handleModeChange(String value);
    void handleConfigSave();
    void handleConfigReset();
    void handleInfo();
    void handleStatus();
    void handleHelp();
    
    // NUEVOS: Radio Profiles handlers
    void handleConfigRadioProfile(String value);
    void handleRadioProfileCustom(String param, String value);
    void handleRadioProfileApply();
    void handleRadioProfileStatus();
    
    /*
     * MÉTODOS UTILITARIOS DE STRINGS
     */
    String getRoleString(DeviceRole role);
    String getStateString(SystemState state);
    String getDataModeString(DataDisplayMode mode);
    String getRegionString(LoRaRegion region);
};

/*
 * INSTANCIA GLOBAL
 */
extern ConfigManager configManager;

#endif