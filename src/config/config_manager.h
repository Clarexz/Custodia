/*
 * CONFIG_MANAGER.H - Sistema de Configuración Modular
 * 
 * MODULARIZADO: Separación entre core y comandos
 * Declaraciones centralizadas para todo el sistema de configuración
 * 
 * BLOQUE D: Agregadas declaraciones para comandos NETWORK_*
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

// ============== NETWORK CHANNELS EEPROM KEYS ==============
// IMPORTANTE: Basado en pattern de Meshtastic NodeDB.cpp
#define EEPROM_CHANNEL_COUNT_KEY    "net_ch_cnt"     // Número de channels configurados
#define EEPROM_ACTIVE_CHANNEL_KEY   "active_ch"      // Índice del channel activo
#define EEPROM_CHANNEL_NAME_PREFIX  "ch_"            // ch_0_name, ch_1_name, etc.
#define EEPROM_CHANNEL_PSK_PREFIX   "psk_"           // psk_0, psk_1, etc.

// Límites de channels (siguiendo Meshtastic limits)
#define MAX_CHANNELS                8                // Máximo 8 channels como Meshtastic
#define MAX_CHANNEL_NAME_LENGTH     30               // Nombre máximo 11 chars
#define MAX_PSK_LENGTH             32               // PSK máximo 32 bytes (AES-256)

// =========================================================

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
    //Basado en Meshtastic Channels.cpp structure
    uint8_t activeChannelIndex;                    // Índice del channel activo (0-7)
    char activeChannelName[MAX_CHANNEL_NAME_LENGTH + 1]; // Nombre del channel activo
    bool hasActiveChannel;                         // Flag si hay channel configurado
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

    // Estructura simple para channels en memoria
    struct SimpleChannel {
        String name;
        String psk;
        bool active;
    };
    
    // Array de channels y contadores
    SimpleChannel networkChannels[MAX_CHANNELS];
    int channelCount;
    int activeChannelIndex;
    
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
    
    // ========== BLOQUE D: NETWORK SECURITY HANDLERS ==========
    void handleNetworkCreate(String params);
    void handleNetworkJoin(String params);
    void handleNetworkList();
    void handleNetworkInfo(String channelName);
    void handleNetworkDelete(String channelName);
    void handleTestEncrypt();
    void handleTestDecrypt();
    void handleStats();
    // ==========================================================
    
    // Radio Profiles handlers
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
    String getActiveChannelName();
};

/*
 * INSTANCIA GLOBAL
 */
extern ConfigManager configManager;

#endif