# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-Fase%203%20COMPLETADA%20+%20MESH%20FUNCIONAL-brightgreen)
![Versión](https://img.shields.io/badge/Versión-2.0.0-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Algoritmo](https://img.shields.io/badge/Algoritmo-Meshtastic%20Completo-purple)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa con **algoritmo completo de Meshtastic**. Implementación que integra el sistema de Managed Flood Routing.

### Objetivo Principal
Crear un firmware unificado que permita configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa, con configuración completa via comandos seriales y operación autónoma **con mesh routing**.

### Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **GPS simulado** con múltiples modos
- **Sistema de estados** bien definido
- **NUEVO: Algoritmo Meshtastic integrado**

---

## Hardware Soportado

| Componente | Modelo | Estado | Función |
|------------|--------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Implementado | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | **MESH FUNCIONAL (faltan pruebas)** | Comunicación mesh de largo alcance (faltan pruebas) |
| **Pantalla** | ESP32-compatible TFT | Futuro | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Simulado | Geolocalización |

---

## Roles del Sistema

### TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- **Mesh routing con CLIENT priority**
- **Estado actual**: **Mesh operativo con algoritmo Meshtastic**

### REPEATER
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos con **ROUTER priority**
- **SNR-based intelligent delays**
- **Estado actual**: **Lógica de Meshtastic implementada**

### RECEIVER
- Recibe y visualiza posiciones GPS de toda la red
- **Duplicate detection y mesh statistics**
- Puede configurar otros dispositivos remotamente (futuro)
- **Estado actual**: **Mesh operativo, pendiente mapas offline**

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

**Valores implementados (exactos de Meshtastic):**
- **CW Min/Max**: 2/8 con slot time de 10ms
- **SNR Range**: -20dB a +15dB
- **Max Hops**: 3 saltos
- **Packet Memory**: 100 packets, 5 minutos
- **Role Priority**: REPEATER = ROUTER priority

### **FASE 4: Mesh Routing Avanzado** - 90% COMPLETADO
- [x] ~~Algoritmo de routing básico~~ **Meshtastic completo**
- [x] ~~Manejo de colisiones con random delay~~ **SNR-based delays**
- [x] ~~Implementación de TTL~~ **Hop limit management**
- [x] **Estadísticas básicas de red** **Implementado**

**Lo que falta (opcional):**
- [ ] Estadísticas avanzadas de topología de red
- [ ] Mesh health monitoring
- [ ] Network discovery automático

### **FASE 5: Características Avanzadas** - PENDIENTE

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
└── lora.cpp         # Sistema LoRa + Mesh - Implementación

platformio.ini        # Configuración del proyecto PlatformIO
README.md             # Esta documentación
```

### Flujo de Datos Enhanced

```
[Startup] → [Config Manager] → [EEPROM Load] → [Estado Inicial]
                                    ↓
[CONFIG_MODE] ←→ [Comandos Seriales] ←→ [Validación]
      ↓                                      ↓
[CONFIG_SAVE] → [EEPROM] → [START] → [RUNNING]
                                        ↓
                              [GPS Manager] → [Simulación]
                                        ↓
                              [LoRa Manager] → [Meshtastic Algorithm]
                                        ↓
                     [Duplicate Detection] → [SNR Delays] → [Mesh Routing]
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
- Simulación GPS realista
- Cálculos geográficos (Haversine)
- Formateo de datos para transmisión
- Diferentes modos de movimiento

#### **LoRaManager** (`lora.h/cpp`) - **ENHANCED CON MESHTASTIC**
**Responsabilidades principales:**
- Hardware SX1262 management
- Protocolo de packets optimizado
- **Algoritmo completo de Meshtastic:**
  - `wasSeenRecently()` - Duplicate detection
  - `perhapsRebroadcast()` - Smart rebroadcast
  - `getTxDelayMsecWeighted()` - SNR-based delays
  - Role-based priority management
- Estadísticas mesh completas

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

**⚡ Role Priority:**
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
# El nuevo código debería auto-reparar esto:
1. El sistema detecta automáticamente LoRa no inicializado
2. Reinicializa LoRa después de configuración
3. Si persiste, verificar hardware
```

### Problemas de Mesh

**"Packets recibidos: 0"**
```bash
# Verificaciones:
1. Ambos dispositivos en misma frecuencia (915 MHz)
2. Mismos parámetros LoRa (SF7, BW125, etc.)
3. Distancia apropiada (empezar <5 metros)
4. Verificar que ambos estén transmitiendo
```

---

## Roadmap del Proyecto

### Próximos Pasos (Fase 5)

| Característica | Tiempo estimado | Prioridad |
|----------------|-----------------|-----------|
| **Remote configuration** | 2-3 días | Alta |
| **Battery monitoring** | 1 día | Media |
| **Power optimization** | 2 días | Media |
| **Mapas offline** | 1 semana | Baja |
| **Network analytics** | 3 días | Baja |

### Mejoras Arquitecturales Planeadas

**Modularización del código:**
```
src/
├── lora/
│   ├── radio_interface.h/cpp
│   └── lora_manager.h/cpp  
├── mesh/
│   ├── flooding_router.h/cpp
│   ├── packet_manager.h/cpp
│   └── mesh_types.h
└── main.cpp
```

---

## Recursos Adicionales

### Documentación Técnica
- [Meshtastic Firmware Repository](https://github.com/meshtastic/firmware) - Código fuente original
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)

### Algoritmo Meshtastic
- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Estado de Cumplimiento de Requirements

### Key Requirements COMPLETADOS

| Requirement | Estado | Notas |
|-------------|--------|-------|
| **Fork Meshtastic y simplificar** | Implementado desde cero | Acercamiento más eficiente con algoritmo completo |
| **Compatibilidad ESP32-S3** | Completado | XIAO ESP32S3 específicamente |
| **Easy to flash** | Completado | PlatformIO setup sencillo |
| **Remove unnecessary features** | Completado | Solo funciones esenciales |
| **Lightweight packet format** | Completado | `deviceID,lat,lon,timestamp` |
| **Mesh rebroadcast** | **SUPERADO** | **Algoritmo completo Meshtastic** |
| **EEPROM storage** | Completado | Device ID y GPS interval |

### System Behavior COMPLETADOS

| Behavior | Estado | Notas |
|----------|--------|-------|
| **Fully off-grid** | Completado | Sin WiFi/cellular/internet |
| **Configurable GPS intervals** | Completado | 5-3600 segundos |
| **Autonomous trackers** | Completado | Operación independiente |
| **Real mesh testing** | **FUNCIONANDO** | **Demo exitosa realizada** |

### System Features EN PROGRESO

| Feature | Estado | Progreso |
|---------|--------|----------|
| **Remote configuration** | Fase 5 | Arquitectura lista |
| **Battery monitoring** | Fase 5 | Framework preparado |
| **Offline maps visualization** | Fase 5 | Para RECEIVER |
| **Theoretical mesh estimation** | Fase 5 | Cálculo basado en Meshtastic |

---
---

## Equipo

- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Lead Developer/Engineer**: Bryan Caleb Martinez Cavazos

---

**Última actualización**: Junio 19, 2025  
**Estado del proyecto**: **FASE 3 COMPLETADA + MESH FUNCIONAL**