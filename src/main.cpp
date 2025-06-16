#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Pin del LED para indicadores visuales
#define LED_PIN 21

// Declaraciones de funciones
void handleOperativeMode();
void handleTrackerMode();
void handleRepeaterMode();
void handleReceiverMode();

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  
  // Esperar a que se establezca la conexión serial
  delay(2000);
  
  // Configurar LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Inicializar sistema de configuración
  configManager.begin();
}

void loop() {
  // Procesar comandos seriales si están disponibles
  configManager.processSerialInput();
  
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
      handleOperativeMode();
      break;
      
    case STATE_SLEEP:
      // Modo sleep (implementaremos después)
      delay(100);
      break;
      
    default:
      delay(100);
      break;
  }
}

void handleOperativeMode() {
  DeviceConfig config = configManager.getConfig();
  
  switch (config.role) {
    case ROLE_TRACKER:
      handleTrackerMode();
      break;
      
    case ROLE_REPEATER:
      handleRepeaterMode();
      break;
      
    case ROLE_RECEIVER:
      handleReceiverMode();
      break;
      
    default:
      // Rol no válido, volver a configuración
      configManager.setState(STATE_CONFIG_MODE);
      Serial.println("[ERROR] Rol no válido. Entrando en modo configuración.");
      break;
  }
}

void handleTrackerMode() {
  // Placeholder para modo TRACKER
  // LED parpadeo rápido para indicar transmisión GPS
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  
  DeviceConfig config = configManager.getConfig();
  Serial.println("[TRACKER] Simulando transmisión GPS cada " + String(config.gpsInterval) + "s");
  
  // Esperar intervalo GPS
  delay(config.gpsInterval * 1000);
}

void handleRepeaterMode() {
  // Placeholder para modo REPEATER
  // LED parpadeando muy rápido para indicar actividad de repetidor
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  
  Serial.println("[REPEATER] Escuchando y retransmitiendo...");
  delay(1000);
}

void handleReceiverMode() {
  // Placeholder para modo RECEIVER
  // LED encendido constante para indicar recepción activa
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("[RECEIVER] Escuchando posiciones GPS...");
  delay(2000);
} 