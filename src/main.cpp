#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif
#include "config/config_manager.h"
#include "gps/gps_manager.h"
#include "battery/battery_manager.h"
#include "lora.h"
#include "roles/role_manager.h"
#include "serial/serial_handler.h"
#include "serial/remote_commands.h"

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    delay(2000);
    
    // Configurar LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Inicializar sistema de configuración
    configManager.begin();
    
    // Inicializar GPS y LoRa solo si la configuración es válida
    if (configManager.isConfigValid()) {
        roleManager.initializeForRole();
    }
}

void loop() {
    // Procesar comandos seriales
    serialHandler.processSerialInput();
    
    // Verificar si necesitamos inicializar LoRa después de configuración
    if (configManager.getState() == STATE_RUNNING && !roleManager.isLoRaInitialized()) {
        if (configManager.isConfigValid()) {
            Serial.println("[MAIN] === INICIALIZANDO SISTEMAS DESPUÉS DE CONFIGURACIÓN ===");
            roleManager.initializeForRole();
            Serial.println("[MAIN] Sistemas inicializados exitosamente");
        }
    }
    
    // Actualizar GPS y LoRa si está habilitado
    if (configManager.getState() == STATE_RUNNING && roleManager.isLoRaInitialized()) {
        // La lógica GPS actual sólo avanza los contadores cuando se envía un
        // packet, por lo que no es necesario actualizarla constantemente.
        loraManager.update();
        
        // Procesar mensajes entrantes (importante para configuración remota)
        // TODOS los roles deben procesar mensajes entrantes
        remoteCommands.processIncomingMessages();
    }
    
    // Comportamiento según el estado actual
    switch (configManager.getState()) {
        case STATE_CONFIG_MODE:
            // En modo configuración, parpadear LED lentamente
            digitalWrite(LED_PIN, HIGH);
            delay(1000);
            digitalWrite(LED_PIN, LOW);
            delay(1000);
            break;
            
        case STATE_RUNNING:
            // En modo operativo, ejecutar lógica según el rol
            if (roleManager.isLoRaInitialized()) {
                roleManager.handleOperativeMode();
            } else {
                Serial.println("[MAIN] Esperando inicialización de sistemas...");
                delay(1000);
            }
            break;
            
        case STATE_SLEEP:
            delay(100);
            break;
            
        default:
            delay(100);
            break;
    }
}
