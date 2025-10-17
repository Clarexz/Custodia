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
#include "user_logic_solarnode.h"

// Demora de inicio segura para permitir enumeración USB y estabilizar potencia
static unsigned long initNotBeforeMs = 0;

void setup() {
    user_logic::begin();

    // Inicializar comunicación serial
    Serial.begin(115200);
    {
        unsigned long t0 = millis();
        while (!Serial && (millis() - t0) < 6000) {
            delay(10);
        }
        delay(200); // margen adicional
    }
    // Posponer inicialización de LoRa/GPS unos segundos tras el boot
    initNotBeforeMs = millis() + 4000; // 4s extra
    
    // Configurar LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Inicializar sistema de configuración
    configManager.begin();
    
    // Diferir la inicialización; se hará en loop() pasado el tiempo de seguridad
}

void loop() {
    user_logic::handle();

    // Procesar comandos seriales
    serialHandler.processSerialInput();
    
    // Verificar si necesitamos inicializar LoRa después de configuración
    if (configManager.getState() == STATE_RUNNING && !roleManager.isLoRaInitialized()) {
        // Respetar ventana de arranque seguro antes de inicializar LoRa/GPS
        if (millis() < initNotBeforeMs) {
            delay(50);
            return;
        }
        if (configManager.isConfigValid()) {
            Serial.println("[MAIN] === INICIALIZANDO SISTEMAS DESPUÉS DE CONFIGURACIÓN ===");
            roleManager.initializeForRole();
            Serial.println("[MAIN] Sistemas inicializados exitosamente");
        }
    }
    
    // Actualizar GPS y LoRa si está habilitado
    if (configManager.getState() == STATE_RUNNING && roleManager.isLoRaInitialized()) {
        // Alimentar constantemente el parser del GPS (necesario para TinyGPSPlus)
        gpsManager.update();
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
