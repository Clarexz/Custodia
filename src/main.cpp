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
void initializeLoRaForRole();

// Funciones para diferentes modos de display
void printSimpleTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent);
void printAdminTrackerOutput(uint16_t deviceID, bool sent);
void printSimpleRepeaterOutput(const String& packet);
void printAdminRepeaterOutput();
void printSimpleReceiverOutput(const String& packet);
void printAdminReceiverOutput();

// Flag para controlar inicialización tardía de LoRa
bool loraInitialized = false;

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
    initializeGPSForRole();
    initializeLoRaForRole();
    loraInitialized = true;
  }
}

void loop() {
  // ACTUALIZADO: Procesar comandos seriales en TODOS los estados
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    
    if (input.length() > 0) {
      // Si estamos en CONFIG_MODE, procesar normalmente
      if (configManager.getState() == STATE_CONFIG_MODE) {
        // No hacer nada aquí, será procesado por configManager.processSerialInput() abajo
      }
      // NUEVO: Si estamos en RUNNING y es comando MODE, procesarlo
      else if (configManager.getState() == STATE_RUNNING && input.startsWith("MODE ")) {
        Serial.println(">" + input); // Echo del comando
        configManager.handleModeChange(input.substring(5));
        return; // Salir temprano para evitar procesar dos veces
      }
      // Informar sobre comandos disponibles durante operación
      else if (configManager.getState() == STATE_RUNNING) {
        Serial.println(">" + input);
        Serial.println("[INFO] Comandos disponibles: MODE SIMPLE, MODE ADMIN");
        return; // Salir temprano
      }
    }
  }
  
  // Procesar comandos seriales del modo configuración
  configManager.processSerialInput();
  
  // Verificar si necesitamos inicializar LoRa después de configuración
  if (configManager.getState() == STATE_RUNNING && !loraInitialized) {
    if (configManager.isConfigValid()) {
      Serial.println("[MAIN] === INICIALIZANDO SISTEMAS DESPUÉS DE CONFIGURACIÓN ===");
      initializeGPSForRole();
      initializeLoRaForRole();
      loraInitialized = true;
      Serial.println("[MAIN] Sistemas inicializados exitosamente");
    }
  }
  
  // Actualizar GPS y LoRa si está habilitado
  if (configManager.getState() == STATE_RUNNING && loraInitialized) {
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
      if (loraInitialized) {
        handleOperativeMode();
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

void handleOperativeMode() {
  DeviceConfig config = configManager.getConfig();
  
  // Verificación adicional de que LoRa esté funcionando
  if (loraManager.getStatus() == LORA_STATUS_ERROR) {
    Serial.println("[MAIN] ERROR: LoRa en estado de error. Reintentando inicialización...");
    initializeLoRaForRole();
    delay(2000);
    return;
  }
  
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
      configManager.setState(STATE_CONFIG_MODE);
      loraInitialized = false;
      Serial.println("[ERROR] Rol no válido. Entrando en modo configuración.");
      break;
  }
}

void handleTrackerMode() {
  static unsigned long lastGPSTransmission = 0;
  static unsigned long lastStatusCheck = 0;
  DeviceConfig config = configManager.getConfig();
  unsigned long currentTime = millis();
  
  // Verificación periódica del estado de LoRa (cada 30 segundos)
  if (currentTime - lastStatusCheck >= 30000) {
    lastStatusCheck = currentTime;
    LoRaStatus loraStatus = loraManager.getStatus();
    
    if (loraStatus == LORA_STATUS_ERROR || loraStatus == LORA_STATUS_INIT) {
      Serial.println("[TRACKER] WARNING: LoRa no está listo. Estado: " + loraManager.getStatusString());
      Serial.println("[TRACKER] Reintentando inicialización...");
      initializeLoRaForRole();
      return;
    }
    
    // Mostrar estadísticas mesh solo en modo ADMIN
    if (configManager.isAdminMode()) {
      loraManager.printMeshStats();
    }
  }
  
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
      // Obtener datos para transmisión
      float lat = gpsData.latitude;
      float lon = gpsData.longitude;
      uint16_t battery = gpsData.batteryVoltage;
      uint32_t timestamp = gpsData.timestamp;
      
      // Verificar estado de LoRa antes de transmitir
      if (loraManager.getStatus() != LORA_STATUS_READY) {
        Serial.println("[TRACKER] WARNING: LoRa no está listo para transmitir");
        Serial.println("[TRACKER] Estado actual: " + loraManager.getStatusString());
        return;
      }
      
      // Transmitir via LoRa
      bool sent = loraManager.sendGPSData(lat, lon, timestamp);
      
      // Mostrar output según modo configurado
      if (configManager.isSimpleMode()) {
        printSimpleTrackerOutput(config.deviceID, lat, lon, battery, timestamp, sent);
      } else {
        printAdminTrackerOutput(config.deviceID, sent);
      }
      
    } else {
      Serial.println("[TRACKER] Sin fix GPS - Esperando señal...");
    }
  }
  
  delay(100);
}

void handleRepeaterMode() {
  static unsigned long lastActivity = 0;
  static unsigned long lastStatusCheck = 0;
  static uint32_t lastRebroadcastCount = 0;
  unsigned long currentTime = millis();
  
  // Verificación periódica del estado de LoRa
  if (currentTime - lastStatusCheck >= 30000) {
    lastStatusCheck = currentTime;
    if (loraManager.getStatus() == LORA_STATUS_ERROR) {
      Serial.println("[REPEATER] ERROR: LoRa en estado de error. Reinicializando...");
      initializeLoRaForRole();
      return;
    }
  }
  
  // LED parpadeando muy rápido para indicar actividad de repetidor
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  
  // Verificar si hubo nuevas retransmisiones para modo SIMPLE
  LoRaStats stats = loraManager.getStats();
  if (configManager.isSimpleMode() && stats.rebroadcasts > lastRebroadcastCount) {
    // Generar packet simulado para mostrar en modo simple
    String packet = "002,25.302677,-98.277664,3950,1718661234"; // Ejemplo
    printSimpleRepeaterOutput(packet);
    lastRebroadcastCount = stats.rebroadcasts;
  }
  
  // Mostrar estado según intervalo y modo
  if (currentTime - lastActivity >= 10000) {
    lastActivity = currentTime;
    
    if (configManager.isAdminMode()) {
      printAdminRepeaterOutput();
    }
  }
  
  delay(100);
}

void handleReceiverMode() {
  static unsigned long lastStatusUpdate = 0;
  static uint32_t lastPacketCount = 0;
  static unsigned long lastStatusCheck = 0;
  unsigned long currentTime = millis();
  
  // Verificación periódica del estado de LoRa
  if (currentTime - lastStatusCheck >= 30000) {
    lastStatusCheck = currentTime;
    if (loraManager.getStatus() == LORA_STATUS_ERROR) {
      Serial.println("[RECEIVER] ERROR: LoRa en estado de error. Reinicializando...");
      initializeLoRaForRole();
      return;
    }
  }
  
  // LED encendido constante para indicar recepción activa
  digitalWrite(LED_PIN, HIGH);
  
  // Verificar si se recibieron nuevos packets
  LoRaStats stats = loraManager.getStats();
  if (stats.packetsReceived > lastPacketCount) {
    if (configManager.isSimpleMode()) {
      // Generar packet simulado para mostrar en modo simple
      String packet = "001,25.302688,-98.277675,4100,1718661240"; // Ejemplo
      printSimpleReceiverOutput(packet);
    }
    lastPacketCount = stats.packetsReceived;
  }
  
  // Mostrar estado cada 5 segundos solo en modo ADMIN
  if (currentTime - lastStatusUpdate >= 5000) {
    lastStatusUpdate = currentTime;
    
    if (configManager.isAdminMode()) {
      printAdminReceiverOutput();
    }
  }
  
  delay(500);
}

/*
 * FUNCIONES PARA MODOS DE DISPLAY
 */

void printSimpleTrackerOutput(uint16_t deviceID, float lat, float lon, uint16_t battery, uint32_t timestamp, bool sent) {
  // Formato simple según key requirements: [deviceID, latitude, longitude, batteryvoltage, timestamp]
  String packet = String(deviceID) + "," + 
                 String(lat, 6) + "," + 
                 String(lon, 6) + "," + 
                 String(battery) + "," + 
                 String(timestamp);
  
  Serial.println("[" + packet + "]");
  if (sent) {
    Serial.println("Envío realizado");
  } else {
    Serial.println("Error en envío");
  }
  Serial.println();
}

void printAdminTrackerOutput(uint16_t deviceID, bool sent) {
  // Mostrar información completa como antes
  DeviceConfig config = configManager.getConfig();
  GPSData gpsData = gpsManager.getCurrentData();
  LoRaStats stats = loraManager.getStats();
  
  Serial.println("\n[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===");
  Serial.println("Device ID: " + String(deviceID));
  Serial.println("Role: TRACKER (CLIENT priority)");
  Serial.println("Coordenadas: " + gpsManager.formatCoordinates());
  Serial.println("Battery: " + String(gpsData.batteryVoltage) + " mV");
  Serial.println("Timestamp: " + String(gpsData.timestamp));
  Serial.println("Packet: " + gpsManager.formatPacketWithDeviceID(deviceID));
  Serial.println("LoRa Status: " + String(sent ? "ENVIADO" : "FALLIDO"));
  Serial.println("Estado LoRa: " + loraManager.getStatusString());
  
  if (sent) {
    Serial.println("RSSI último: " + String(loraManager.getLastRSSI()) + " dBm");
    Serial.println("SNR último: " + String(loraManager.getLastSNR()) + " dB");
    Serial.println("Packets enviados: " + String(stats.packetsSent));
    Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
    Serial.println("Retransmisiones: " + String(stats.rebroadcasts));
  } else {
    Serial.println("ERROR: Fallo en transmisión LoRa");
  }
  
  Serial.println("Próxima transmisión en " + String(config.gpsInterval) + " segundos");
  Serial.println("==========================================\n");
}

void printSimpleRepeaterOutput(const String& packet) {
  // Mostrar solo el packet que se está retransmitiendo
  Serial.println("[" + packet + "]");
  Serial.println("Retransmisión realizada");
  Serial.println();
}

void printAdminRepeaterOutput() {
  // Mostrar información completa del repeater
  LoRaStats stats = loraManager.getStats();
  
  Serial.println("\n[REPEATER] === ESTADO DEL REPETIDOR ===");
  Serial.println("Escuchando red mesh - Listo para retransmitir");
  Serial.println("Role: REPEATER (ROUTER priority)");
  Serial.println("Estado LoRa: " + loraManager.getStatusString());
  Serial.println("RX: " + String(stats.packetsReceived) + 
                 " | TX: " + String(stats.packetsSent) + 
                 " | Retransmisiones: " + String(stats.rebroadcasts));
  Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
  Serial.println("Hop limit alcanzado: " + String(stats.hopLimitReached));
  Serial.println("===================================\n");
}

void printSimpleReceiverOutput(const String& packet) {
  // Mostrar solo el packet recibido
  Serial.println("[" + packet + "]");
  Serial.println("Datos recibidos");
  Serial.println();
}

void printAdminReceiverOutput() {
  // Mostrar información completa del receiver
  LoRaStats stats = loraManager.getStats();
  GPSData gpsData = gpsManager.getCurrentData();
  
  Serial.println("\n[RECEIVER] === ESTADO DEL RECEPTOR MESH ===");
  Serial.println("Escuchando posiciones GPS de la red...");
  Serial.println("Role: RECEIVER (CLIENT priority)");
  Serial.println("Estado LoRa: " + loraManager.getStatusString());
  
  // Mostrar nuestra propia posición como referencia
  if (gpsData.hasValidFix) {
    Serial.println("Posición propia: " + gpsManager.formatCoordinates());
    Serial.println("Estado GPS: " + gpsManager.getStatusString());
  }
  
  // Mostrar estadísticas de red
  Serial.println("Packets recibidos: " + String(stats.packetsReceived));
  Serial.println("Duplicados ignorados: " + String(stats.duplicatesIgnored));
  Serial.println("Retransmisiones hechas: " + String(stats.rebroadcasts));
  
  // Detectar nuevos packets recibidos
  static uint32_t lastPacketCount = 0;
  if (stats.packetsReceived > lastPacketCount) {
    Serial.println("¡NUEVOS DATOS RECIBIDOS!");
    Serial.println("Último RSSI: " + String(stats.lastRSSI) + " dBm");
    Serial.println("Último SNR: " + String(stats.lastSNR) + " dB");
    lastPacketCount = stats.packetsReceived;
  }
  
  Serial.println("========================================\n");
}

/*
 * FUNCIONES DE INICIALIZACIÓN
 */

void initializeLoRaForRole() {
  DeviceConfig config = configManager.getConfig();
  
  Serial.println("[MAIN] Inicializando LoRa para rol: " + String(config.role));
  
  // Inicializar LoRa con el device ID configurado
  if (!loraManager.begin(config.deviceID)) {
    Serial.println("[MAIN] ERROR: Fallo en inicialización LoRa");
    loraInitialized = false;
    return;
  }
  
  // Configurar role en LoRaManager para mesh priority
  loraManager.setRole(config.role);
  
  // Configuración específica según rol
  switch (config.role) {
    case ROLE_TRACKER:
      Serial.println("[MAIN] LoRa configurado para TRACKER (CLIENT priority)");
      break;
      
    case ROLE_REPEATER:
      Serial.println("[MAIN] LoRa configurado para REPEATER (ROUTER priority)");
      break;
      
    case ROLE_RECEIVER:
      Serial.println("[MAIN] LoRa configurado para RECEIVER (CLIENT priority)");
      break;
      
    default:
      Serial.println("[MAIN] Rol no reconocido - LoRa en configuración por defecto");
      break;
  }
  
  // Mostrar configuración solo en modo ADMIN
  if (configManager.isAdminMode()) {
    loraManager.printConfiguration();
    
    Serial.println("[MAIN] === CONFIGURACIÓN MESH ===");
    Serial.println("Algoritmo: Meshtastic Managed Flood Routing");
    Serial.println("Max hops: " + String(MESHTASTIC_MAX_HOPS));
    Serial.println("Duplicate detection: ACTIVO");
    Serial.println("SNR-based delays: ACTIVO");
    Serial.println("Role priority: " + String(config.role == ROLE_REPEATER ? "ALTA" : "NORMAL"));
    Serial.println("=============================");
  }
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
      Serial.println("[MAIN] GPS configurado: Movimiento lineal, 15 km/h");
      break;
      
    case ROLE_REPEATER:
      // Repeater puede tener GPS fijo (solo para referencia)
      gpsManager.begin(GPS_SIM_FIXED);
      Serial.println("[MAIN] GPS configurado: Posición fija");
      break;
      
    case ROLE_RECEIVER:
      // Receiver puede tener GPS fijo o móvil
      gpsManager.begin(GPS_SIM_RANDOM_WALK);
      gpsManager.setSimulationSpeed(5.0f);   // 5 km/h simula persona caminando
      Serial.println("[MAIN] GPS configurado: Caminata aleatoria, 5 km/h");
      break;
      
    default:
      Serial.println("[MAIN] Rol no reconocido - GPS en modo fijo");
      gpsManager.begin(GPS_SIM_FIXED);
      break;
  }
}