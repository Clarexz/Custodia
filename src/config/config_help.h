/*
 * CONFIG_HELP.H - Sistema de Ayuda y Documentación
 * 
 * MODULARIZADO: Declaraciones para el sistema de ayuda
 * En este caso es mínimo ya que la ayuda se maneja en config_commands.cpp
 */

#ifndef CONFIG_HELP_H
#define CONFIG_HELP_H

#include <Arduino.h>

/*
 * CONSTANTES DE AYUDA
 */

// Mensajes de ayuda que podrían ser reutilizados
#define HELP_ROLE_OPTIONS "TRACKER|REPEATER|RECEIVER"
#define HELP_DEVICE_ID_RANGE "1-999"
#define HELP_GPS_INTERVAL_RANGE "5-3600"
#define HELP_MAX_HOPS_RANGE "1-10"
#define HELP_DATA_MODE_OPTIONS "SIMPLE|ADMIN"
#define HELP_REGION_OPTIONS "US|EU|CH|AS|JP"

/*
 * FUNCIONES DE AYUDA (si se necesitaran en el futuro)
 */

// Por ahora, toda la ayuda está integrada en handleHelp() en config_commands.cpp
// Este archivo está preparado para futuras expansiones del sistema de ayuda

#endif