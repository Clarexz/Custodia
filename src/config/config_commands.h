/*
 * CONFIG_COMMANDS.H - Declaraciones para Manejadores de Comandos
 * 
 * MODULARIZADO: Todas las funciones de procesamiento de comandos
 * separadas del core para mejor organización
 */

#ifndef CONFIG_COMMANDS_H
#define CONFIG_COMMANDS_H

#include "config_manager.h"

/*
 * FORWARD DECLARATIONS
 * 
 * Estas funciones están implementadas en config_commands.cpp
 * pero necesitan ser accesibles desde ConfigManager
 */

// NOTA: Las declaraciones de los métodos de CommandHandler están en config_manager.h
// porque son métodos de la clase ConfigManager. Este archivo es para funciones auxiliares
// si las necesitamos en el futuro.

// Timeout en milisegundos para confirmaciones de usuario
#define CONFIRMATION_TIMEOUT 10000

#endif