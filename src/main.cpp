#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gps.h"
#include "lora.h"

// Pin del LED para indicadores visuales
#define LED_PIN 21

// Declaraciones de funciones
void handleOperativeMode();
void handleTrackerMode();
void handleRepeaterMode();
void handleReceiverMode();
void initializeGPSForRole();
void initializeLoRaForRole();  // Agregar esta declaración

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
  
  // Inicializar GPS y LoRa solo si la configuración es válida
  if (configManager.isConfigValid()) {
    initializeGPSForRole();
    initializeLoRaForRole();
  }
}

void loop() {
  // Procesar comandos seriales si están disponibles
  configManager.processSerialInput();
  
  // Actualizar GPS y LoRa si está habilitado
  if (configManager.getState() == STATE_RUNNING) {
    gpsManager.update();
    loraManager.update();
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
      // Obtener datos GPS actuales para transmisión LoRa
      float lat = gpsData.latitude;
      float lon = gpsData.longitude;
      uint32_t timestamp = gpsData.timestamp;
      
      // Transmitir via LoRa
      bool sent = loraManager.sendGPSData(lat, lon, timestamp);
      
      // Mostrar información de transmisión
      Serial.println("\n[TRACKER] === TRANSMISIÓN GPS + LoRa ===");
      Serial.println("Device ID: " + String(config.deviceID));
      Serial.println("Coordenadas: " + gpsManager.formatCoordinates());
      Serial.println("Timestamp: " + String(timestamp));
      Serial.println("Packet: " + String(config.deviceID) + "," + gpsManager.formatForTransmission());
      Serial.println("LoRa Status: " + String(sent ? "ENVIADO" : "FALLIDO"));
      if (sent) {
        Serial.println("RSSI: " + String(loraManager.getLastRSSI()) + " dBm");
      }
      Serial.println("Próxima transmisión en " + String(config.gpsInterval) + " segundos");
      Serial.println("=======================================\n");
    } else {
      Serial.println("[TRACKER] Sin fix GPS - Esperando señal...");
    }
  }
  
  // Pequeña pausa para no sobrecargar el CPU
  delay(100);
}

void handleRepeaterMode() {
  static unsigned long lastActivity = 0;
  unsigned long currentTime = millis();
  
  // LED parpadeando muy rápido para indicar actividad de repetidor
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  
  // Mostrar estado cada 10 segundos
  if (currentTime - lastActivity >= 10000) {
    lastActivity = currentTime;
    Serial.println("[REPEATER] Escuchando red mesh - Listo para retransmitir");
    Serial.println("[REPEATER] Estado LoRa: " + loraManager.getStatusString());
    
    // Mostrar estadísticas cada vez
    LoRaStats stats = loraManager.getStats();
    if (stats.packetsReceived > 0 || stats.packetsSent > 0) {
      Serial.println("[REPEATER] RX: " + String(stats.packetsReceived) + 
                     " | TX: " + String(stats.packetsSent) + 
                     " | Perdidos: " + String(stats.packetsLost));
    }
  }
  
  delay(100);  // Pausa pequeña para no saturar
}

void handleReceiverMode() {
  static unsigned long lastStatusUpdate = 0;
  static uint32_t lastPacketCount = 0;
  unsigned long currentTime = millis();
  
  // LED encendido constante para indicar recepción activa
  digitalWrite(LED_PIN, HIGH);
  
  // Mostrar estado cada 5 segundos
  if (currentTime - lastStatusUpdate >= 5000) {
    lastStatusUpdate = currentTime;
    
    Serial.println("\n[RECEIVER] === ESTADO DEL RECEPTOR ===");
    Serial.println("Escuchando posiciones GPS de la red...");
    Serial.println("Estado LoRa: " + loraManager.getStatusString());
    
    // Mostrar nuestra propia posición como referencia
    GPSData gpsData = gpsManager.getCurrentData();
    if (gpsData.hasValidFix) {
      Serial.println("Posición propia: " + gpsManager.formatCoordinates());
      Serial.println("Estado GPS: " + gpsManager.getStatusString());
    }
    
    // Mostrar estadísticas de red
    LoRaStats stats = loraManager.getStats();
    Serial.println("Packets recibidos: " + String(stats.packetsReceived));
    
    // Detectar nuevos packets recibidos
    if (stats.packetsReceived > lastPacketCount) {
      Serial.println("¡NUEVOS DATOS RECIBIDOS!");
      Serial.println("Último RSSI: " + String(stats.lastRSSI) + " dBm");
      Serial.println("Último SNR: " + String(stats.lastSNR) + " dB");
      lastPacketCount = stats.packetsReceived;
    }
    
    Serial.println("====================================\n");
  }
  
  delay(500);
}

/*
 * INICIALIZACIÓN DE LORA SEGÚN ROL
 */
void initializeLoRaForRole() {
  DeviceConfig config = configManager.getConfig();
  
  Serial.println("[MAIN] Inicializando LoRa para rol: " + String(config.role));
  
  // Inicializar LoRa con el device ID configurado
  if (!loraManager.begin(config.deviceID)) {
    Serial.println("[MAIN] ERROR: Fallo en inicialización LoRa");
    return;
  }
  
  // Configuración específica según rol
  switch (config.role) {
    case ROLE_TRACKER:
      Serial.println("[MAIN] LoRa configurado para TRACKER");
      // Los trackers pueden usar configuración por defecto
      break;
      
    case ROLE_REPEATER:
      Serial.println("[MAIN] LoRa configurado para REPEATER");
      // Los repeaters podrían usar configuración optimizada para retransmisión
      break;
      
    case ROLE_RECEIVER:
      Serial.println("[MAIN] LoRa configurado para RECEIVER");
      // Los receivers podrían usar configuración optimizada para recepción
      break;
      
    default:
      Serial.println("[MAIN] Rol no reconocido - LoRa en configuración por defecto");
      break;
  }
  
  // Mostrar configuración actual
  loraManager.printConfiguration();
}

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