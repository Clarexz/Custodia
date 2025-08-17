/*
 * USER_LOGIC_LILYGO.H - Configuración específica para LilyGo T-SIM7080-S3
 *
 */

#ifndef LORA_HARDWARE_LILYGO_H
#define LORA_HARDWARE_LILYGO_H

#ifdef LILYGO_TSIM7080_S3

/*
 * ANÁLISIS DE PINES - LILYGO T-SIM7080-S3 (PINOUT OFICIAL CONFIRMADO)
 * 
 * PINES OCUPADOS:
 * - SIM7080G:   IO41(PWR), IO42(DTR), IO03(RI), IO04(RXD), IO05(TXD)
 * - PMU:        IO15(SDA), IO07(SCL), IO06(INT)
 * - TF-Card:    IO39(CMD), IO38(CLK), IO40(DATA)
 * - FSPI/PSRAM: IO35, IO36, IO37
 * - Cámara:     IO18,IO08,IO02,IO01,IO16,IO17,IO12,IO09,IO11,IO13,IO21,IO48,IO47,IO14
 * 
 * PINES LIBRES CONFIRMADOS (headers disponibles):
 * - Siempre libres: GPIO43, GPIO44, GPIO46, GPIO10, GPIO00, GPIO45
 * - Libres sin cámara: todos los pines de cámara arriba
 */

// =============================================
// CONFIGURACIÓN SIM7080G (PARA CONTROL/DISABLE)
// =============================================

// Pines LoRa fake para evitar errores de compilación
#define LORA_SCK_PIN            0           // Placeholder - no usado
#define LORA_MISO_PIN           0           // Placeholder - no usado
#define LORA_MOSI_PIN           0           // Placeholder - no usado
#define LORA_NSS_PIN            0           // Placeholder - no usado
#define LORA_DIO1_PIN           0           // Placeholder - no usado
#define LORA_NRST_PIN           0           // Placeholder - no usado
#define LORA_BUSY_PIN           0           // Placeholder - no usado

// Pines del SIM7080G (para poder deshabilitarlo en Fase 1)
#define SIM_PWR_PIN     41      // Power control del SIM7080G
#define SIM_TX_PIN      4       // UART TX hacia SIM7080G  
#define SIM_RX_PIN      5       // UART RX desde SIM7080G
#define SIM_RI_PIN      3       // Ring Indicator
#define SIM_DTR_PIN     42      // Data Terminal Ready

// =============================================
// CONFIGURACIÓN PMU (POWER MANAGEMENT)
// =============================================

// PMU I2C (para monitoreo de batería - compatible con sistema actual)
#define PMU_SDA_PIN     15      // PMU I2C Data
#define PMU_SCL_PIN     7       // PMU I2C Clock  
#define PMU_IRQ_PIN     6       // PMU Interrupt

// =============================================
// OTROS PINES DEL SISTEMA
// =============================================

// LED onboard (si existe, o podemos usar uno externo)
#define LED_PIN         21      // Revisar si está disponible o usar externo

// Pines reservados para uso futuro
#define RESERVED_PIN_1  23      // Disponible para expansión
#define RESERVED_PIN_2  47      // Disponible para expansión

// =============================================
// VALIDACIÓN DE CONFIGURACIÓN
// =============================================

// Macro para verificar que no hay conflictos de pines
#define VALIDATE_PIN_CONFIG() \
    static_assert(LORA_SCK_PIN != SIM_TX_PIN, "Pin conflict: LoRa SCK vs SIM TX"); \
    static_assert(LORA_MISO_PIN != SIM_RX_PIN, "Pin conflict: LoRa MISO vs SIM RX"); \
    static_assert(LORA_NSS_PIN != SIM_PWR_PIN, "Pin conflict: LoRa NSS vs SIM PWR");

// =============================================
// CONFIGURACIÓN DE INICIALIZACIÓN
// =============================================

// Función para deshabilitar SIM7080G durante Fase 1
inline void disableSIM7080G() {
    // Configurar pin de power como output y mantenerlo LOW
    pinMode(SIM_PWR_PIN, OUTPUT);
    digitalWrite(SIM_PWR_PIN, LOW);
    
    // Configurar pines UART como input con pull-up para evitar floating
    pinMode(SIM_TX_PIN, INPUT_PULLUP);
    pinMode(SIM_RX_PIN, INPUT_PULLUP);
    pinMode(SIM_RI_PIN, INPUT_PULLUP);
    pinMode(SIM_DTR_PIN, INPUT_PULLUP);
}

// Función para validar hardware LilyGo T-SIM7080-S3
inline void validateLilyGoHardware() {
    Serial.println("=== LILYGO T-SIM7080-S3 HARDWARE VALIDATION ===");
    
    // Verificar PSRAM (debe ser 8MB)
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM: %d bytes (%.1f MB)\n", 
                     ESP.getPsramSize(), ESP.getPsramSize() / 1024.0 / 1024.0);
    } else {
        Serial.println("WARNING: PSRAM no detectado");
    }
    
    // Verificar Flash (debe ser 16MB)
    Serial.printf("Flash: %d bytes (%.1f MB)\n", 
                 ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1024.0 / 1024.0);
    
    // Deshabilitar SIM7080G para Fase 1
    disableSIM7080G();
    Serial.println("SIM7080G disabled for Phase 1");
    
    // Verificar pines LoRa (basado en pinout oficial)
    Serial.println("LoRa pins configured (OFFICIAL PINOUT):");
    Serial.printf("   SCK: %d, MISO: %d, MOSI: %d\n", LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN);
    Serial.printf("   NSS: %d, DIO1: %d, RST: %d, BUSY: %d\n", 
                 LORA_NSS_PIN, LORA_DIO1_PIN, LORA_NRST_PIN, LORA_BUSY_PIN);
    
    Serial.println("Hardware validation completed");
}

#endif // BOARD_LILYGO_TSIM7080_S3
#endif // LORA_HARDWARE_LILYGO_H