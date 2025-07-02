/*
 * LORA_HARDWARE.H - Configuración de Hardware SX1262
 * 
 * MODULARIZADO: Configuración específica de hardware, pines y parámetros
 * de radio extraídos del lora.h original
 */

#ifndef LORA_HARDWARE_H
#define LORA_HARDWARE_H

#include <Arduino.h>
#include <RadioLib.h>

/*
 * CONFIGURACIÓN DE HARDWARE PARA XIAO ESP32S3 + WIO SX1262
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

// Configuración optimizada para velocidad vs alcance
#define LORA_BANDWIDTH      125.0f  // kHz - Balance velocidad/alcance
#define LORA_SPREADING_FACTOR   7   // SF7 = Rápido, SF12 = Largo alcance
#define LORA_CODING_RATE        5   // 4/5 = Buena corrección de errores
#define LORA_TX_POWER          14   // dBm - Potencia de transmisión
#define LORA_PREAMBLE_LENGTH    8   // Símbolos de preámbulo
#define LORA_SYNC_WORD     0x12     // Palabra de sincronización personalizada

#endif