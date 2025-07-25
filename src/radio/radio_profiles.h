/*
 * RADIO_PROFILES.H - Sistema de Perfiles LoRa Predefinidos
 * 
 * BLOQUE E: Configuraciones optimizadas para diferentes escenarios
 * Basado en trade-offs de alcance, velocidad, batería y congestión
 */

#ifndef RADIO_PROFILES_H
#define RADIO_PROFILES_H

#include <Arduino.h>

/*
 * PERFILES DISPONIBLES
 */
enum RadioProfile {
    PROFILE_DESERT_LONG_FAST = 0,    // Máximo alcance, campo abierto
    PROFILE_MOUNTAIN_STABLE = 1,     // Obstáculos, condiciones adversas
    PROFILE_URBAN_DENSE = 2,         // Alta velocidad, muchos dispositivos
    PROFILE_MESH_MAX_NODES = 3,      // Balance para redes grandes
    PROFILE_CUSTOM_ADVANCED = 4      // Configuración manual del usuario
};

/*
 * ESTRUCTURA DE CONFIGURACIÓN DE PERFIL
 */
struct RadioProfileConfig {
    // Identificación
    RadioProfile profileId;
    const char* name;
    const char* description;
    
    // Parámetros LoRa
    uint8_t spreadingFactor;    // SF7-SF12
    float bandwidth;            // kHz: 125, 250, 500
    uint8_t codingRate;         // 4/5=5, 4/6=6, 4/7=7, 4/8=8
    int8_t txPower;            // dBm: 2-20
    uint8_t preambleLength;     // Símbolos de preámbulo
    
    // Características calculadas (solo lectura)
    uint16_t approxRange;       // Metros (estimado)
    uint16_t airtimeMs;         // ms para packet de 44 bytes
    uint8_t batteryRating;      // 1-10 (10=mejor batería)
    uint8_t speedRating;        // 1-10 (10=más rápido)
    
    // Casos de uso recomendados
    const char* useCase;
    const char* tradeOffs;
};

/*
 * CONFIGURACIONES PREDEFINIDAS
 */

// PERFIL 1: DESERT_LONG_FAST
// Objetivo: Máximo alcance en terreno abierto
// Trade-off: Alcance máximo vs velocidad y batería
#define DESERT_SF           12      // Máxima sensibilidad
#define DESERT_BW           125.0f  // Balance estándar
#define DESERT_CR           5       // 4/5 - Mínima redundancia para velocidad
#define DESERT_POWER        20      // Máxima potencia
#define DESERT_PREAMBLE     8       // Estándar

// PERFIL 2: MOUNTAIN_STABLE
// Objetivo: Robustez en condiciones adversas con obstáculos
// Trade-off: Estabilidad vs velocidad
#define MOUNTAIN_SF         10      // Alto para penetración
#define MOUNTAIN_BW         125.0f  // Estándar para estabilidad
#define MOUNTAIN_CR         6       // 4/6 - Más corrección de errores
#define MOUNTAIN_POWER      17      // Alto pero no máximo
#define MOUNTAIN_PREAMBLE   12      // Más largo para sincronización

// PERFIL 3: URBAN_DENSE
// Objetivo: Alta velocidad en entornos con muchos dispositivos
// Trade-off: Velocidad y capacidad vs alcance
#define URBAN_SF            7       // Mínimo para máxima velocidad
#define URBAN_BW            250.0f  // Doble ancho para más velocidad
#define URBAN_CR            5       // 4/5 - Mínima redundancia
#define URBAN_POWER         10      // Menor para reducir interferencia
#define URBAN_PREAMBLE      6       // Mínimo para velocidad

// PERFIL 4: MESH_MAX_NODES
// Objetivo: Balance para redes mesh grandes (20-30 nodos)
// Trade-off: Equilibrio entre todos los factores
#define MESH_SF             8       // Buen balance alcance/velocidad
#define MESH_BW             125.0f  // Estándar
#define MESH_CR             5       // 4/5 - Balance
#define MESH_POWER          14      // Potencia media
#define MESH_PREAMBLE       8       // Estándar

// PERFIL 5: CUSTOM_ADVANCED
// Valores por defecto para configuración manual
#define CUSTOM_SF           8       // Valor inicial razonable
#define CUSTOM_BW           125.0f  // Estándar
#define CUSTOM_CR           5       // 4/5
#define CUSTOM_POWER        14      // Medio
#define CUSTOM_PREAMBLE     8       // Estándar

/*
 * CLASE PARA GESTIÓN DE PERFILES
 */
class RadioProfileManager {
private:
    RadioProfile currentProfile;
    RadioProfileConfig customConfig;
    
public:
    /*
     * CONSTRUCTOR
     */
    RadioProfileManager();
    
    /*
     * MÉTODOS PRINCIPALES
     */
    
    // Obtener configuración de un perfil
    RadioProfileConfig getProfileConfig(RadioProfile profile);
    
    // Aplicar perfil al sistema LoRa
    bool applyProfile(RadioProfile profile);
    
    // Configurar perfil custom
    bool setCustomParameter(const String& param, float value);
    
    /*
     * GETTERS Y SETTERS
     */
    RadioProfile getCurrentProfile() { return currentProfile; }
    String getProfileName(RadioProfile profile);
    String getProfileDescription(RadioProfile profile);
    
    /*
     * CÁLCULOS Y ESTIMACIONES
     */
    
    // Calcular airtime aproximado para packet de tamaño dado
    uint16_t calculateAirtime(RadioProfile profile, uint8_t packetSize = 44);
    
    // Estimar alcance aproximado
    uint16_t estimateRange(RadioProfile profile);
    
    // Calcular rating de batería (1-10)
    uint8_t calculateBatteryRating(RadioProfile profile);
    
    // Calcular rating de velocidad (1-10)
    uint8_t calculateSpeedRating(RadioProfile profile);
    
    /*
     * INFORMACIÓN Y DOCUMENTACIÓN
     */
    
    // Mostrar información completa de un perfil
    void printProfileInfo(RadioProfile profile);
    
    // Mostrar todos los perfiles disponibles
    void printAllProfiles();
    
    // Mostrar comparación entre perfiles
    void printProfileComparison();
    
    /*
     * VALIDACIÓN
     */
    
    // Validar parámetros de configuración
    bool isValidConfiguration(uint8_t sf, float bw, uint8_t cr, int8_t power);
    
    // Verificar compatibilidad con región actual
    bool isCompatibleWithRegion(RadioProfile profile);
};

/*
 * INSTANCIA GLOBAL
 */
extern RadioProfileManager radioProfileManager;

/*
 * CONSTANTES DE CÁLCULO
 */

// Para estimación de alcance (basado en free space path loss)
#define BASE_RANGE_SF7      1000    // metros con SF7 en condiciones ideales
#define RANGE_GAIN_PER_SF   1.58f   // ~4dB gain = ~1.58x range por cada SF
#define POWER_RANGE_FACTOR  1.12f   // Factor de alcance por cada dB de potencia

// Para cálculo de airtime (fórmulas aproximadas)
#define SYMBOL_TIME_BASE    1000    // Factor base para cálculo de tiempo de símbolo

// Ratings de referencia
#define MIN_AIRTIME_MS      50      // Para rating 10 de velocidad
#define MAX_AIRTIME_MS      2500    // Para rating 1 de velocidad
#define MIN_POWER_DBM       2       // Para rating 10 de batería
#define MAX_POWER_DBM       20      // Para rating 1 de batería

#endif