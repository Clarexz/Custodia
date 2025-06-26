# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-Fase%204%20COMPLETADA-brightgreen)
![Versión](https://img.shields.io/badge/Versión-2.1.0-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Algoritmo](https://img.shields.io/badge/Algoritmo-Meshtastic%20Completo-purple)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa con **algoritmo de Meshtastic**. Implementación que integra el sistema de Managed Flood Routing con **modos de visualización flexibles** y **comandos en tiempo real**.

### Objetivo Principal
Crear un firmware unificado que permita configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa, con configuración completa via comandos seriales, operación autónoma **con mesh routing**, y **modos de visualización simple/admin**.

### Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **GPS simulado** con múltiples modos
- **Sistema de estados** bien definido
- **Algoritmo Meshtastic integrado**
- **Modos de visualización flexibles**
- **Comandos en tiempo real** sin reinicio

---

## Hardware Soportado

| Componente | Modelo | Estado | Función |
|------------|--------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Implementado | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | **MESH FUNCIONAL** | Comunicación mesh de largo alcance |
| **Pantalla** | ESP32-compatible TFT | Futuro | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Simulado | Geolocalización |

---

## Roles del Sistema

### TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- **Mesh routing con CLIENT priority**
- **Battery monitoring integrado**
- **Estado actual**: **Mesh operativo con algoritmo Meshtastic + Modos visualización**

### REPEATER
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos con **ROUTER priority**
- **SNR-based intelligent delays**
- **Estado actual**: **Lógica de Meshtastic implementada + Modos visualización**

### RECEIVER
- Recibe y visualiza posiciones GPS de toda la red
- **Duplicate detection y mesh statistics**
- Puede configurar otros dispositivos remotamente (futuro)
- **Estado actual**: **Mesh operativo + Modos visualización**

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

### **FASE 2: GPS Simulado** - COMPLETADA

**Funcionalidades implementadas:**
- [x] **Sistema GPS** con 5 modos de simulación
- [x] **Integración con configuración** existente
- [x] **Diferentes comportamientos GPS** según rol del dispositivo
- [x] **Transmisión periódica** de coordenadas en modo TRACKER
- [x] **Validación y formateo** de datos GPS
- [x] **Fórmula de Haversine** para cálculos geográficos precisos

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

### **FASE 4: Modos de Visualización** - COMPLETADA

**Funcionalidades implementadas:**

- [x] **Modo SIMPLE** - Vista limpia con solo packet format
- [x] **Modo ADMIN** - Vista completa con estadísticas mesh y diagnóstico
- [x] **Comando CONFIG_DATA_MODE** - Configuración durante setup
- [x] **Comandos MODE en tiempo real** - Cambio sin reinicio durante operación
- [x] **Auto-guardado automático** - Cambios persisten en EEPROM
- [x] **Battery monitoring** - Voltage de batería en cada packet
- [x] **Soporte todos los roles** - TRACKER, REPEATER, RECEIVER

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
   cd custom-meshtastic-custom
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

### Hardware Setup LoRa

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

## Comandos Disponibles

### Comandos de Configuración

```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>  # Configurar rol del dispositivo
CONFIG_DEVICE_ID <1-999>                 # Configurar ID único
CONFIG_GPS_INTERVAL <5-3600>             # Intervalo GPS en segundos
CONFIG_MAX_HOPS <1-10>                   # Máximo saltos en mesh
CONFIG_DATA_MODE <SIMPLE|ADMIN>          # Modo de visualización
CONFIG_SAVE                              # Guardar configuración
CONFIG_RESET                             # Resetear configuración
INFO                                     # Información del dispositivo
STATUS                                   # Estado y configuración actual
START                                    # Iniciar modo operativo
HELP                                     # Mostrar todos los comandos
```

### Comandos Durante Operación (Nuevos)

```bash
MODE SIMPLE                              # Cambiar a vista simple
MODE ADMIN                               # Cambiar a vista completa
```

**IMPORTANTE**: Los comandos `MODE` funcionan **en tiempo real** sin necesidad de reiniciar el dispositivo y se guardan automáticamente en EEPROM.

---

## Modos de Visualización

### Modo SIMPLE

**Comando durante configuración**: `CONFIG_DATA_MODE SIMPLE`  
**Comando durante operación**: `MODE SIMPLE`

**Propósito**: Vista limpia que muestra solo el packet básico según los key requirements del proyecto.

**Formato del packet**: `[deviceID, latitude, longitude, batteryvoltage, timestamp]`

**Ejemplo de salida**:
```
[001,25.302677,-98.277664,4100,1718661234]
Envío realizado
```

### Modo ADMIN

**Comando durante configuración**: `CONFIG_DATA_MODE ADMIN`  
**Comando durante operación**: `MODE ADMIN`

**Propósito**: Vista completa con toda la información de mesh, estadísticas y diagnóstico.

**Ejemplo de salida**:
```
[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Coordenadas: 25.302677,-98.277664
Battery: 4100 mV
Timestamp: 1718661234
Packet: [001,25.302677,-98.277664,4100,1718661234]
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

### Cambio de Modos

```bash
# Durante configuración
config> CONFIG_DATA_MODE SIMPLE
[OK] Modo de datos configurado: SIMPLE

# Durante operación (SIN REINICIAR)
MODE ADMIN
[OK] Modo cambiado a: ADMIN
[INFO] Mostrando información completa

MODE SIMPLE
[OK] Modo cambiado a: SIMPLE
[INFO] Mostrando solo packets básicos
```

**Los cambios con `MODE` se guardan automáticamente y persisten entre reinicios.**

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

config> CONFIG_DATA_MODE SIMPLE
[OK] Modo de datos configurado: SIMPLE
[INFO] Se mostrará solo: [deviceID, latitude, longitude, batteryvoltage, timestamp]

config> STATUS
=== CONFIGURACIÓN ACTUAL ===
Rol: TRACKER
Device ID: 1
Intervalo GPS: 15 segundos
Máximo saltos: 3
Modo de datos: SIMPLE
============================

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

config> START
[OK] Iniciando modo operativo...
[INFO] Comandos disponibles durante operación: MODE SIMPLE, MODE ADMIN

[001,25.302677,-98.277664,4100,1718661234]
Envío realizado

MODE ADMIN
[OK] Modo cambiado a: ADMIN

# Ahora muestra información completa
[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===
Device ID: 1
Role: TRACKER (CLIENT priority)
...
```

### Configuración de otros Roles

**REPEATER:**
```bash
CONFIG_ROLE REPEATER
CONFIG_DEVICE_ID 002
CONFIG_GPS_INTERVAL 30
CONFIG_DATA_MODE ADMIN
CONFIG_SAVE
START
```

**RECEIVER:**
```bash
CONFIG_ROLE RECEIVER
CONFIG_DEVICE_ID 003
CONFIG_GPS_INTERVAL 15
CONFIG_DATA_MODE SIMPLE
CONFIG_SAVE
START
```

---

## Packet Format

### Formato Exacto

**Estructura**: `[deviceID, latitude, longitude, batteryvoltage, timestamp]`

**Ejemplo**: `[001,25.302677,-98.277664,4100,1718661234]`

### Campos del Packet

| Campo | Descripción | Formato | Ejemplo |
|-------|-------------|---------|---------|
| `deviceID` | ID único del dispositivo | 001-999 | `001` |
| `latitude` | Latitud en grados decimales | 6 decimales | `25.302677` |
| `longitude` | Longitud en grados decimales | 6 decimales | `-98.277664` |
| `batteryvoltage` | Voltaje de batería | milivolts (mV) | `4100` |
| `timestamp` | Unix timestamp | segundos | `1718661234` |

### Battery Monitoring

- **Voltage inicial**: ~4200mV (4.2V) - Batería llena
- **Descarga simulada**: ~0.1mV por minuto
- **Voltage mínimo**: 3200mV (3.2V) - Batería descargada
- **Reset**: Reiniciar dispositivo para battery a nivel alto

---

### Flujo de Datos Enhanced

```
[Startup] → [Config Manager] → [EEPROM Load] → [Estado Inicial]
                                    ↓
[CONFIG_MODE] ←→ [Comandos Seriales] ←→ [Validación]
      ↓                                      ↓
[CONFIG_SAVE] → [EEPROM] → [START] → [RUNNING]
                                        ↓
                              [GPS Manager] → [Simulación + Battery]
                                        ↓
                              [LoRa Manager] → [Meshtastic Algorithm]
                                        ↓
                     [Duplicate Detection] → [SNR Delays] → [Mesh Routing]
                                        ↓
                              [Data Mode] → [SIMPLE | ADMIN Display]
```

### Módulos Principales

#### **ConfigManager** (`config.h/cpp`)
**Responsabilidades:**
- Procesamiento de comandos seriales
- Persistencia en EEPROM
- Validación de parámetros
- Control de estados del sistema
- **Gestión de modos de visualización**
- **Comandos en tiempo real**

#### **GPSManager** (`gps.h/cpp`)
**Responsabilidades:**
- Simulación GPS realista
- Cálculos geográficos (Haversine)
- Formateo de datos para transmisión
- Diferentes modos de movimiento
- **Battery monitoring integrado**

#### **(Agregar info de la carpeta lora)**
---

## Algoritmo Meshtastic Implementado

### Core Components

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

### Problemas de Comandos

**"Comando CONFIG_DATA_MODE no reconocido"**
```bash
# Verificar versión del firmware
1. Comando INFO para ver versión (debe ser 2.1.0+)
2. Comando correcto: CONFIG_DATA_MODE (no CONFIG_DATA)
3. Opciones: CONFIG_DATA_MODE SIMPLE o CONFIG_DATA_MODE ADMIN
```

**"Comando MODE no funciona"**
```bash
# Verificar estado del sistema:
1. MODE solo funciona después de START (durante operación)
2. Durante configuración usar: CONFIG_DATA_MODE
3. Comandos válidos: MODE SIMPLE o MODE ADMIN
4. Los cambios se guardan automáticamente
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

### Problemas de Visualización

**"Battery voltage siempre igual"**
```bash
# Es simulación - battery desciende gradualmente:
1. Voltage inicial: ~4200mV (4.2V)
2. Descarga: ~0.1mV por minuto
3. Voltage mínimo: 3200mV (3.2V)
4. Reiniciar para resetear battery a nivel alto
```

**"Modo SIMPLE no muestra solo el packet"**
```bash
# Verificar configuración:
1. Comando STATUS para ver modo actual
2. Cambiar con MODE SIMPLE durante operación
3. O CONFIG_DATA_MODE SIMPLE durante configuración
4. Cambios se guardan automáticamente
```

---

## Fase 5 (En progreso)

| Característica | Tiempo estimado | Prioridad |
|----------------|-----------------|-----------|
| **Remote configuration** | 1 día | Media |
| **Power optimization** | 1 día | Media |

---

## Documentación Adicional

### Recursos Útiles
- [Documentación de Meshtastic](https://meshtastic.org/) - Referencia del protocolo original
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)

### Algoritmo Meshtastic
- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Estado de Cumplimiento de Requirements

### Key Requirements COMPLETADOS Y SUPERADOS

| Requirement | Estado | Notas |
|-------------|--------|-------|
| **Fork Meshtastic y simplificar** | Implementado desde cero | Acercamiento más eficiente con algoritmo completo |
| **Compatibilidad ESP32-S3** | Completado | XIAO ESP32S3 específicamente |
| **Easy to flash** | Completado | PlatformIO setup sencillo |
| **Remove unnecessary features** | Completado | Solo funciones esenciales |
| **Lightweight packet format** | Completado | **[deviceID,lat,lon,battery,timestamp]** |
| **Mesh rebroadcast** | Completado | **Algoritmo completo Meshtastic** |
| **Battery monitoring** | **AGREGADO** | **Voltage en cada packet** |
| **EEPROM storage** | Completado | Device ID, GPS interval, y **data mode** |

### System Behavior COMPLETADOS

| Behavior | Estado | Notas |
|----------|--------|-------|
| **Fully off-grid** | Completado | Sin WiFi/cellular/internet |
| **Configurable GPS intervals** | Completado | 5-3600 segundos |
| **Autonomous trackers** | Completado | Operación independiente |
| **Real mesh testing** | **FUNCIONANDO** | **Demo exitosa realizada** |

### System Features COMPLETADAS

| Feature | Estado | Progreso |
|---------|--------|----------|
| **Data visualization modes** | **COMPLETADO** | **SIMPLE + ADMIN modes** |
| **Real-time mode switching** | **COMPLETADO** | **MODE commands** |
| **Battery monitoring** | **COMPLETADO** | **Integrated in packets** |
| **Auto-save configuration** | **COMPLETADO** | **EEPROM persistence** |

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

## Equipo

- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Lead Developer/Engineer**: Bryan Caleb Martinez Cavazos

---
**Versión del firmware**: 2.1.0  
**Estado del proyecto**: **FASE 4 COMPLETADA - MODOS SIMPLE/ADMIN + COMANDOS TIEMPO REAL**