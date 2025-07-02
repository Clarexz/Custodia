/*
 * LORA.H - Sistema LoRa Enhanced (SOLUCIÓN DEFINITIVA)
 * 
 * CORREGIDO: Include config_manager.h PRIMERO para evitar problemas de dependencia
 */

#ifndef LORA_H
#define LORA_H

/*
 * SOLUCIÓN: INCLUIR CONFIG PRIMERO
 * 
 * Al incluir config_manager.h primero, DeviceRole se define ANTES
 * de que lora_types.h y lora_manager.h lo necesiten
 */

// 1. Config PRIMERO (define DeviceRole)
#include "config/config_manager.h"

// 2. Tipos LoRa (ahora DeviceRole ya está definido)
#include "lora/lora_types.h"

// 3. Hardware config
#include "lora/lora_hardware.h"

// 4. Manager principal (ahora DeviceRole está disponible)
#include "lora/lora_manager.h"

/*
 * COMPATIBILIDAD TOTAL
 * 
 * Todo el código existente sigue funcionando exactamente igual
 */

#endif