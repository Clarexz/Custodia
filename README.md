# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-Modularizaci%C3%B3n%20en%20Progreso-orange)
![Versión](https://img.shields.io/badge/Versión-2.1.0-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Algoritmo](https://img.shields.io/badge/Algoritmo-Meshtastic%20Completo-purple)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa con **algoritmo de Meshtastic**. Implementación que integra el sistema de Managed Flood Routing desarrollado específicamente para asset tracking.

### Objetivo Principal
Crear un firmware unificado que permita configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa, con configuración completa via comandos seriales y operación autónoma **con mesh routing**.

### Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **GPS simulado realista** con múltiples modos
- **Sistema de estados** bien definido
- **Algoritmo Meshtastic integrado** con SNR-based delays y role priority

---

## Hardware Soportado

| Componente | Modelo | Estado | Función |
|------------|--------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Implementado | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | **MESH FUNCIONAL** | Comunicación mesh de largo alcance |
| **Pantalla** | ESP32-compatible TFT | Planificado | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Simulado | Geolocalización |

**Conexiones XIAO ESP32S3 + Wio SX1262:**
```
XIAO ESP32S3    →    Wio SX1262
GPIO 7 (SCK)    →    SCK
GPIO 8 (MISO)   →    MISO  
GPIO 9 (MOSI)   →    MOSI
GPIO 41 (CS)    →    NSS
GPIO 39         →    DIO1
GPIO 42         →    RESET
GPIO 40         →    BUSY
3.3V            →    VCC
GND             →    GND
```

---

## Roles del Sistema

### TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- **Mesh routing con CLIENT priority**
- **Estado actual**: Mesh operativo con algoritmo Meshtastic completo

### REPEATER
- Actúa como extensor de rango para la red mesh
- Retransmite mensajes de otros dispositivos con **ROUTER priority**
- **SNR-based intelligent delays** para optimización de red
- **Estado actual**: Lógica de Meshtastic implementada y funcional

### RECEIVER
- Recibe y visualiza posiciones GPS de toda la red mesh
- **Duplicate detection y mesh statistics** integradas
- Puede configurar otros dispositivos remotamente (planificado)
- **Estado actual**: Mesh operativo, mapas offline en desarrollo

---

## Estado Actual del Proyecto

### **FASE 1: Sistema de Configuración Serial** - COMPLETADA

**Funcionalidades implementadas:**
- [x] **Parser robusto** de comandos seriales con validación completa
- [x] **Persistencia en EEPROM** usando Preferences
- [x] **Sistema de estados** (BOOT, CONFIG_MODE, RUNNING, SLEEP)
- [x] **Validación automática** de configuración completa
- [x] **Manejo de errores** con confirmaciones de seguridad
- [x] **Documentación integrada** (comando HELP)
- [x] **Indicadores visuales LED** según estado/rol

**Comandos disponibles:**
```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>  # Configurar rol del dispositivo
CONFIG_DEVICE_ID <1-999>                 # Configurar ID único
CONFIG_GPS_INTERVAL <5-3600>             # Intervalo GPS en segundos
CONFIG_MAX_HOPS <1-10>                   # Máximo saltos en mesh
CONFIG_SAVE                              # Guardar configuración
CONFIG_RESET                             # Resetear configuración
INFO                                     # Información del dispositivo
STATUS                                   # Estado y configuración actual
START                                    # Iniciar modo operativo
HELP                                     # Mostrar todos los comandos
```

### **FASE 2: GPS Simulado** - COMPLETADA

**Funcionalidades implementadas:**
- [x] **Sistema GPS completo** con 5 modos de simulación
- [x] **Integración con configuración** existente
- [x] **Diferentes comportamientos GPS** según rol del dispositivo
- [x] **Transmisión periódica** de coordenadas en modo TRACKER
- [x] **Validación y formateo** de datos GPS
- [x] **Fórmula de Haversine** para cálculos geográficos precisos

**Modos de simulación GPS:**
- **GPS_SIM_FIXED**: Posición fija
- **GPS_SIM_LINEAR**: Movimiento lineal
- **GPS_SIM_CIRCULAR**: Movimiento circular
- **GPS_SIM_RANDOM_WALK**: Caminata aleatoria
- **GPS_SIM_SIGNAL_LOSS**: Pérdida/recuperación de señal

**Formato del packet GPS:**
```
deviceID,latitude,longitude,timestamp
```
**Ejemplo**: `001,25.302677,-98.277664,1718661234`

### **FASE 3: LoRa + Mesh Completo** - COMPLETADA

**Funcionalidades implementadas:**

- [x] **Comunicación LoRa básica** - SX1262 completamente funcional
- [x] **Protocolo de packets optimizado** - Formato binario eficiente
- [x] **Duplicate Packet Detection** - Sistema `wasSeenRecently()` de Meshtastic
- [x] **SNR-based Intelligent Delays** - Nodos lejanos retransmiten primero
- [x] **Role-based Priority** - REPEATERS con 160ms menos delay (como ROUTERS)
- [x] **Managed Flood Routing** - Algoritmo completo de FloodingRouter.cpp
- [x] **Hop Management** - Control de TTL con máximo 3 saltos
- [x] **Mesh Statistics** - Tracking de duplicados, retransmisiones, etc.
- [x] **Contention Window** - Valores exactos de Meshtastic (CWmin=2, CWmax=8)

**Valores implementados (exactos de Meshtastic):**
- **CW Min/Max**: 2/8 con slot time de 10ms
- **SNR Range**: -20dB a +15dB
- **Max Hops**: 3 saltos
- **Packet Memory**: 100 packets, 5 minutos
- **Role Priority**: REPEATER = ROUTER priority

### **FASE 4: Modularización de Código** - COMPLETADA

**Objetivo**: Dividir el archivo `lora.cpp` (800+ líneas) en módulos más pequeños y manejables.

### **FASE 5: Características Avanzadas** - EN PROGRESO

| Característica | Estado | Prioridad | Notas |
|----------------|--------|-----------|-------|
| **Remote configuration** | Planificado | Alta | Receiver-to-tracker commands |
| **Battery monitoring** | Planificado | Media | Voltage reporting en packets |
| **Modo sleep inteligente** | Planificado | Media | Power optimization |
| **Mapas offline en RECEIVER** | Planificado | Baja | Visualización gráfica |
| **Estimación teórica de nodos** | Planificado | Baja | Cálculo basado en Meshtastic |

---

## Instalación y Configuración

### Prerrequisitos
- **Visual Studio Code** con extensión PlatformIO
- **XIAO ESP32S3** microcontrolador
- **Wio SX1262 LoRa module** 
- **Cable USB-C** para conectar el microcontrolador al equipo

### Pasos de Instalación

1. **Clonar el repositorio**
   ```bash
   git clone https://github.com/Clarexz/meshtastic_custom.git
   cd meshtastic_custom
   ```

2. **Abrir en PlatformIO**
   - Abrir VS Code
   - File → Open Folder → Seleccionar carpeta del proyecto

3. **Compilar y subir**
   - Conectar ESP32-S3 via USB-C
   - Click en "Build" en PlatformIO
   - Click en "Upload"

4. **Configuración inicial**
   - Click en "Monitor" (115200 baud)
   - Seguir secuencia de configuración

---

## Guía de Uso

### Configuración de un TRACKER

```bash
# 1. Conectar dispositivo y abrir monitor serial
# 2. El sistema iniciará en modo configuración automáticamente

config> CONFIG_ROLE TRACKER
[OK] Rol configurado: TRACKER

config> CONFIG_DEVICE_ID 001
[OK] Device ID configurado: 1

config> CONFIG_GPS_INTERVAL 15
[OK] Intervalo GPS configurado: 15 segundos

config> STATUS
=== CONFIGURACIÓN ACTUAL ===
Rol: TRACKER
Device ID: 1
Intervalo GPS: 15 segundos
Máximo saltos: 3
============================

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

config> START
[OK] Iniciando modo operativo...
```

### Salida Esperada en Modo TRACKER (Con Mesh)

```
[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Coordenadas: 25.302677,-98.277664
Timestamp: 1718661234
Packet: 1,25.302677,-98.277664,1718661234
LoRa Status: ENVIADO
Estado LoRa: LISTO
RSSI último: -65.5 dBm
SNR último: 10.2 dB
Packets enviados: 15
Duplicados ignorados: 0
Retransmisiones: 3
Próxima transmisión en 15 segundos
==========================================
```

### Configuración de otros Roles

**REPEATER:**
```bash
CONFIG_ROLE REPEATER
CONFIG_DEVICE_ID 002
CONFIG_GPS_INTERVAL 30
CONFIG_SAVE
START
```

**RECEIVER:**
```bash
CONFIG_ROLE RECEIVER
CONFIG_DEVICE_ID 003
CONFIG_GPS_INTERVAL 15
CONFIG_SAVE
START
```

---

## Arquitectura del Código

### Estructura del Proyecto

```
src/
├── main.cpp          # Loop principal y lógica de estados
├── config.h          # Sistema de configuración - Declaraciones
├── config.cpp        # Sistema de configuración - Implementación
├── gps.h            # Sistema GPS - Declaraciones
├── gps.cpp          # Sistema GPS - Implementación
├── lora.h           # Sistema LoRa + Mesh - Declaraciones
├── lora.cpp         # Sistema LoRa + Mesh - Implementación
└── mesh/            # Módulos del algoritmo Meshtastic
    ├── mesh_types.h         # Estructuras compartidas
    ├── packet_manager.h/cpp # Gestión de packets y duplicate detection
    └── flooding_router.h/cpp # Algoritmo de Managed Flood Routing

platformio.ini        # Configuración del proyecto PlatformIO
README.md             # Esta documentación
```

### Módulos Principales

#### **ConfigManager** (`config.h/cpp`)
**Responsabilidades:**
- Procesamiento de comandos seriales
- Persistencia en EEPROM
- Validación de parámetros
- Control de estados del sistema

#### **GPSManager** (`gps.h/cpp`)
**Responsabilidades:**
- Simulación GPS realista con múltiples modos
- Cálculos geográficos (Haversine)
- Formateo de datos para transmisión
- Diferentes patrones de movimiento

## (Falta agregar info de los modulos de lora)

---

## Algoritmo Meshtastic Implementado

### Componentes Principales

**Duplicate Detection:**
```cpp
// Evita loops infinitos con memoria de 100 packets
bool wasSeenRecently(sourceID, packetID);
```

**SNR-based Delays:**
```cpp
// Nodos lejanos (SNR bajo) transmiten primero
uint32_t delay = getTxDelayMsecWeighted(snr, role);
```

**Role Priority:**
```cpp
// REPEATER = ROUTER priority (160ms menos delay)
if (role == ROLE_REPEATER) delay = base_delay;
else delay = base_delay + 160ms;
```

**Hop Management:**
```cpp
// Máximo 3 saltos como Meshtastic
if (packet.hops >= packet.maxHops) drop_packet();
```

### Estadísticas Mesh

```cpp
struct LoRaStats {
    uint32_t packetsSent;       // Packets enviados
    uint32_t packetsReceived;   // Packets recibidos válidos  
    uint32_t duplicatesIgnored; // Duplicados filtrados
    uint32_t rebroadcasts;      // Retransmisiones hechas
    uint32_t hopLimitReached;   // Packets descartados por hop limit
    float lastRSSI, lastSNR;    // Calidad de señal
};
```

---

## Troubleshooting

### Problemas de Upload

**Error: "Could not configure port"**
```bash
# Solución: Secuencia BOOT+RESET
1. Mantener presionado botón BOOT (B)
2. Mientras se mantiene BOOT, presionar y soltar RESET (R)
3. Soltar BOOT
4. Hacer Upload inmediatamente
```

### Problemas de LoRa

**"LoRa Status: FALLIDO"**
```bash
# Verificar conexiones hardware:
1. Revisar todas las conexiones XIAO ↔ SX1262
2. Verificar alimentación 3.3V
3. Comprobar que SX1262 esté correctamente conectado
4. Reset del dispositivo después de configurar
```

**"Sistema no está listo para transmitir"**
```bash
# El sistema detecta automáticamente LoRa no inicializado:
1. Reinicializa LoRa después de configuración
2. Si persiste, verificar hardware y conexiones
3. Verificar que el device ID sea válido (1-999)
```

### Problemas de Mesh

**"Packets recibidos: 0"**
```bash
# Verificaciones:
1. Ambos dispositivos en misma frecuencia (915 MHz)
2. Mismos parámetros LoRa (SF7, BW125, etc.)
3. Distancia apropiada (empezar <5 metros)
4. Verificar que ambos estén transmitiendo
5. Verificar roles: TRACKER envía, RECEIVER recibe
```

### Problemas de Configuración

**"Sin fix GPS - Esperando señal..."**
```bash
# Solución: GPS simulado debería tener fix automático
1. Presionar botón RESET (R)
2. O enviar cualquier comando para volver a CONFIG_MODE
3. Luego START nuevamente
4. Verificar que hasValidFix = true en logs
```

---

## Estado de Cumplimiento de Requirements

### Key Requirements COMPLETADOS

| Requirement | Estado | Notas |
|-------------|--------|-------|
| **Fork Meshtastic y simplificar** | Implementado desde cero | Algoritmo completo con approach más eficiente |
| **Compatibilidad ESP32-S3** | Completado | XIAO ESP32S3 específicamente |
| **Easy to flash** | Completado | PlatformIO setup sencillo |
| **Remote configuration** | Planificado | Será parte de Fase 5 |
| **GitHub documentation** | Completado | README detallado + comentarios en código |
| **Remove unnecessary features** | Completado | Solo funciones esenciales para tracking |
| **Lightweight packet format** | Completado | `deviceID,lat,lon,timestamp` |
| **Mesh rebroadcast** | **SUPERADO** | **Algoritmo completo Meshtastic** |
| **Battery monitoring** | Planificado | Fase 5 |
| **EEPROM storage** | Completado | Device ID y GPS interval |
| **Theoretical mesh estimation** | Planificado | Cálculo basado en Meshtastic |

### System Behavior COMPLETADOS

| Behavior | Estado | Notas |
|----------|--------|-------|
| **Fully off-grid** | Completado | Sin WiFi/cellular/internet |
| **Configurable GPS intervals** | Completado | 5-3600 segundos |
| **Autonomous trackers** | Completado | Operación independiente |
| **Offline maps visualization** | Planificado | Fase 5 - RECEIVER |
| **Real mesh testing** | **FUNCIONANDO** | **Mesh routing operativo** |

### Documentation COMPLETADA

| Document | Estado | Notas |
|----------|--------|-------|
| **User manual** | Completado | Basado en este README |
| **Extended technical** | En progreso | Comentarios detallados en código |
| **Setup procedures** | Completado | Sección de instalación detallada |
| **Packet format details** | Completado | Documentado y implementado |
| **Troubleshooting** | Completado | Sección troubleshooting completa |
| **Clean, commented code** | Completado | Código documentado extensivamente |

---

## Recursos Adicionales

### Documentación Técnica
- [Meshtastic Firmware Repository](https://github.com/meshtastic/firmware) - Código fuente original
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)
- [PlatformIO Documentation](https://docs.platformio.org/)

### Algoritmo Meshtastic
- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Equipo

- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Lead Developer/Engineer**: Bryan Caleb Martinez Cavazos

---