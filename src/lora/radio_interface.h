/*
 * RADIO_INTERFACE.H - Interfaz de Hardware SX1262
 * 
 * Este módulo maneja toda la comunicación de bajo nivel con el chip SX1262,
 * incluyendo inicialización, configuración de parámetros LoRa, transmisión
 * y recepción básica sin lógica de mesh.
 */

#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../mesh/mesh_types.h"

/*
 * CONFIGURACIÓN DE HARDWARE PARA XIAO ESP32S3 + WIO SX1262
 * 
 * Pines oficiales según Meshtastic firmware:
 * Fuente: https://github.com/meshtastic/firmware/variants/seeed_xiao_s3/variant.h
 */

// Configuración oficial de Meshtastic para XIAO S3 + SX1262
#define LORA_SCK_PIN    7   // LORA_SCK
#define LORA_MISO_PIN   8   // LORA_MISO
#define LORA_MOSI_PIN   9   // LORA_MOSI
#define LORA_NSS_PIN    41  // LORA_CS
#define LORA_DIO1_PIN   39  // LORA_DIO1
#define LORA_NRST_PIN   42  // LORA_RESET
#define LORA_BUSY_PIN   40  // SX126X_BUSY

/*
 * CONFIGURACIÓN DE RADIO
 */

// Frecuencia para región de México (915 MHz ISM band)
#define LORA_FREQUENCY      915.0f  // MHz

// Configuración optimizada para velocidad vs alcance
#define LORA_BANDWIDTH      125.0f  // kHz - Balance velocidad/alcance
#define LORA_SPREADING_FACTOR   7   // SF7 = Rápido, SF12 = Largo alcance
#define LORA_CODING_RATE        5   // 4/5 = Buena corrección de errores
#define LORA_TX_POWER          14   // dBm - Potencia de transmisión
#define LORA_PREAMBLE_LENGTH    8   // Símbolos de preámbulo
#define LORA_SYNC_WORD     0x12     // Palabra de sincronización personalizada

/*
 * CONSTANTES DE TIMEOUT Y CONFIGURACIÓN
 */

#define LORA_TX_TIMEOUT         5000    // ms - Timeout para transmisión
#define LORA_RX_TIMEOUT         1000    // ms - Timeout para recepción
#define LORA_INIT_TIMEOUT       10000   // ms - Timeout para inicialización

/*
 * CLASE PRINCIPAL - RadioInterface
 * 
 * Maneja toda la comunicación de bajo nivel con el SX1262:
 * - Inicialización del hardware
 * - Configuración de parámetros LoRa
 * - Transmisión y recepción básica
 * - Gestión de estados del radio
 * - Monitoreo de calidad de señal
 */
class RadioInterface {
private:
    // Instancia del módulo SX1262 usando RadioLib
    SX1262 radio;
    
    // Estado actual del radio
    LoRaStatus status;
    
    // Estadísticas básicas del radio
    LoRaStats stats;
    
    // Configuración actual
    float currentFrequency;
    int8_t currentTxPower;
    float currentBandwidth;
    uint8_t currentSpreadingFactor;
    uint8_t currentCodingRate;
    
    // Buffers de transmisión y recepción
    uint8_t txBuffer[LORA_MAX_PACKET_SIZE];
    uint8_t rxBuffer[LORA_MAX_PACKET_SIZE];
    
public:
    /*
     * CONSTRUCTOR Y DESTRUCTOR
     */
    RadioInterface();
    ~RadioInterface();
    
    /*
     * INICIALIZACIÓN Y CONFIGURACIÓN
     */
    
    // Inicializar el módulo SX1262
    bool begin();
    
    // Configurar todos los parámetros LoRa
    bool configure();
    
    // Test básico de funcionamiento
    bool selfTest();
    
    // Reset del módulo
    void reset();
    
    /*
     * CONFIGURACIÓN DE PARÁMETROS INDIVIDUALES
     */
    
    // Configurar frecuencia
    bool setFrequency(float frequency);
    
    // Configurar potencia de transmisión
    bool setTxPower(int8_t power);
    
    // Configurar bandwidth
    bool setBandwidth(float bandwidth);
    
    // Configurar spreading factor
    bool setSpreadingFactor(uint8_t sf);
    
    // Configurar coding rate
    bool setCodingRate(uint8_t cr);
    
    // Configurar sync word
    bool setSyncWord(uint8_t syncWord);
    
    // Configurar longitud de preámbulo
    bool setPreambleLength(uint8_t length);
    
    /*
     * TRANSMISIÓN Y RECEPCIÓN BÁSICA
     */
    
    // Transmitir datos (blocking)
    bool transmit(const uint8_t* data, size_t length);
    
    // Recibir datos (blocking con timeout)
    int receive(uint8_t* buffer, size_t maxLength);
    
    // Verificar si hay datos disponibles
    bool isPacketAvailable();
    
    // Iniciar modo recepción continua
    bool startReceive();
    
    // Detener recepción
    void stopReceive();
    
    /*
     * CONTROL DE ESTADOS
     */
    
    // Obtener estado actual
    LoRaStatus getStatus() const { return status; }
    
    // Cambiar a modo standby
    bool standby();
    
    // Cambiar a modo sleep
    bool sleep();
    
    // Despertar del sleep
    bool wakeup();
    
    /*
     * MONITOREO DE CALIDAD DE SEÑAL
     */
    
    // Obtener RSSI del último packet recibido
    float getLastRSSI();
    
    // Obtener SNR del último packet recibido
    float getLastSNR();
    
    // Obtener estadísticas del radio
    LoRaStats getStats() const { return stats; }
    
    // Reset de estadísticas
    void resetStats();
    
    /*
     * UTILIDADES Y DIAGNÓSTICO
     */
    
    // Obtener configuración actual
    void getCurrentConfiguration(float* freq, int8_t* power, float* bw, 
                                uint8_t* sf, uint8_t* cr);
    
    // Verificar estado del hardware
    bool isHardwareOK();
    
    // Obtener información del chip
    String getChipInfo();
    
    // Calcular tiempo de transmisión para un packet
    uint32_t calculateAirTime(size_t packetLength);
    
    /*
     * DEBUG Y INFORMACIÓN
     */
    
    // Imprimir configuración actual
    void printConfiguration();
    
    // Imprimir estadísticas
    void printStats();
    
    // Imprimir estado del hardware
    void printHardwareStatus();
    
    // Obtener string del estado actual
    String getStatusString() const;
    
private:
    /*
     * MÉTODOS PRIVADOS
     */
    
    // Inicializar SPI
    bool initSPI();
    
    // Configurar pines de control
    bool setupControlPins();
    
    // Verificar comunicación con el chip
    bool testCommunication();
    
    // Actualizar estadísticas internas
    void updateStats(bool success, uint32_t airTime = 0);
    
    // Convertir código de error de RadioLib a string
    String getErrorString(int errorCode);
    
    // Validar parámetros de configuración
    bool validateConfiguration();
    
    // Calcular configuración óptima automáticamente
    void calculateOptimalSettings();
};

/*
 * INSTANCIA GLOBAL
 * 
 * Se declara aquí y se define en radio_interface.cpp
 */
extern RadioInterface radioInterface;

/*
 * UTILIDADES Y HELPERS
 */

// Convertir RSSI a calidad de señal (0-100%)
uint8_t rssiToSignalQuality(float rssi);

// Convertir SNR a calidad de señal (0-100%)
uint8_t snrToSignalQuality(float snr);

// Verificar si una frecuencia está en rango válido
bool isValidFrequency(float frequency);

// Verificar si un spreading factor es válido
bool isValidSpreadingFactor(uint8_t sf);

// Verificar si un bandwidth es válido
bool isValidBandwidth(float bw);

// Calcular alcance teórico basado en configuración
float calculateTheoreticalRange(int8_t txPower, uint8_t sf, float bw);

// Obtener string descriptivo de configuración LoRa
String getLoRaConfigString(uint8_t sf, float bw, uint8_t cr);

/*
 * CONSTANTES DE CONFIGURACIÓN
 */

// Rangos válidos para parámetros LoRa
#define MIN_FREQUENCY       860.0f   // MHz
#define MAX_FREQUENCY       930.0f   // MHz
#define MIN_TX_POWER        -9       // dBm
#define MAX_TX_POWER        22       // dBm
#define MIN_SPREADING_FACTOR 6       // SF6
#define MAX_SPREADING_FACTOR 12      // SF12
#define MIN_BANDWIDTH       7.8f     // kHz
#define MAX_BANDWIDTH       500.0f   // kHz

// Configuraciones predefinidas optimizadas
struct LoRaPreset {
    const char* name;
    uint8_t spreadingFactor;
    float bandwidth;
    uint8_t codingRate;
    int8_t txPower;
    const char* description;
};

// Presets disponibles
extern const LoRaPreset LORA_PRESET_SHORT_RANGE;
extern const LoRaPreset LORA_PRESET_MEDIUM_RANGE;
extern const LoRaPreset LORA_PRESET_LONG_RANGE;
extern const LoRaPreset LORA_PRESET_VERY_LONG_RANGE;

// Aplicar preset de configuración
bool applyLoRaPreset(const LoRaPreset& preset);

#endif // RADIO_INTERFACE_H