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

// Detección automática de hardware
#ifdef LILYGO_TSIM7080_S3
    #include "user_logic_lilygo.h"
    #define BOARD_HAS_LORA 0
#else
    #include "user_logic.h"
    #define BOARD_HAS_LORA 1
#endif

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