#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gps.h"

// Pin del LED para indicadores visuales
#define LED_PIN 21

// Declaraciones de funciones
void handleOperativeMode();
void handleTrackerMode();
void handleRepeaterMode();
void handleReceiverMode();
void initializeGPSForRole();

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
  
  // Inicializar GPS solo si la configuración es válida
  if (configManager.isConfigValid()) {
    initializeGPSForRole();
  }
}

void loop() {
  // Procesar comandos seriales si están disponibles
  configManager.processSerialInput();
  
  // Actualizar GPS si está habilitado
  if (configManager.getState() == STATE_RUNNING) {
    gpsManager.update();
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
  static unsigned long lastGPSTransmission = 0;
  DeviceConfig config = configManager.getConfig();
  unsigned long currentTime = millis();
  
  // Verificar si es tiempo de transmitir posición GPS
  if (currentTime - lastGPSTransmission >= (config.gpsInterval * 1000)) {
    lastGPSTransmission = currentTime;
    
    // LED parpadeo rápido para indicar transmisión GPS
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    
    // Obtener datos GPS actuales
    GPSData gpsData = gpsManager.getCurrentData();
    
    if (gpsData.hasValidFix) {
      // Mostrar información de transmisión (SIMPLIFICADA)
      Serial.println("\n[TRACKER] === TRANSMISIÓN GPS ===");
      Serial.println("Device ID: " + String(config.deviceID));
      Serial.println("Coordenadas: " + gpsManager.formatCoordinates());
      Serial.println("Timestamp: " + String(gpsData.timestamp));
      Serial.println("Packet: " + String(config.deviceID) + "," + gpsManager.formatForTransmission());
      Serial.println("Próxima transmisión en " + String(config.gpsInterval) + " segundos");
      Serial.println("==============================\n");
    } else {
      Serial.println("[TRACKER] Sin fix GPS - Esperando señal...");
    }
  }
  
  // Pequeña pausa para no sobrecargar el CPU
  delay(100);
}

void handleRepeaterMode() {
  // LED parpadeando muy rápido para indicar actividad de repetidor
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  
  Serial.println("[REPEATER] Escuchando red mesh - Listo para retransmitir");
  delay(2000);
}

void handleReceiverMode() {
  static unsigned long lastStatusUpdate = 0;
  unsigned long currentTime = millis();
  
  // LED encendido constante para indicar recepción activa
  digitalWrite(LED_PIN, HIGH);
  
  // Mostrar estado cada 5 segundos
  if (currentTime - lastStatusUpdate >= 5000) {
    lastStatusUpdate = currentTime;
    
    Serial.println("\n[RECEIVER] === ESTADO DEL RECEPTOR ===");
    Serial.println("Escuchando posiciones GPS de la red...");
    
    // Mostrar nuestra propia posición como referencia
    GPSData gpsData = gpsManager.getCurrentData();
    if (gpsData.hasValidFix) {
      Serial.println("Posición propia: " + gpsManager.formatCoordinates());
      Serial.println("Estado GPS: " + gpsManager.getStatusString());
    }
    
    Serial.println("Dispositivos detectados: 0 (implementar en Fase 4)");
    Serial.println("====================================\n");
  }
  
  delay(1000);
}

/*
 * FUNCIONES AUXILIARES PARA GPS
 */

void initializeGPSForRole() {
  DeviceConfig config = configManager.getConfig();
  
  Serial.println("[MAIN] Inicializando GPS para rol: " + String(config.role));
  
  switch (config.role) {
    case ROLE_TRACKER:
      // Tracker necesita GPS activo con movimiento simulado
      gpsManager.begin(GPS_SIM_LINEAR);
      gpsManager.setSimulationSpeed(15.0f);  // 15 km/h simula vehículo lento
      gpsManager.setUpdateInterval(1000);    // Actualizar cada segundo
      break;
      
    case ROLE_REPEATER:
      // Repeater puede tener GPS fijo (solo para referencia)
      gpsManager.begin(GPS_SIM_FIXED);
      break;
      
    case ROLE_RECEIVER:
      // Receiver puede tener GPS fijo o móvil
      gpsManager.begin(GPS_SIM_RANDOM_WALK);
      gpsManager.setSimulationSpeed(5.0f);   // 5 km/h simula persona caminando
      break;
      
    default:
      Serial.println("[MAIN] Rol no reconocido - GPS en modo fijo");
      gpsManager.begin(GPS_SIM_FIXED);
      break;
  }
}