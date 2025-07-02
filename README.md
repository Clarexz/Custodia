# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-COMPLETAMENTE%20MODULARIZADO-brightgreen)
![Versión](https://img.shields.io/badge/Versión-0.4-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Algoritmo](https://img.shields.io/badge/Algoritmo-Meshtastic%20Completo-purple)
![Regiones](https://img.shields.io/badge/Regiones-US%2FEU%2FCH%2FAS%2FJP-red)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa con **algoritmo completo de Meshtastic**. Implementación **completamente modularizada** que integra el sistema de Managed Flood Routing con **modos de visualización flexibles**, **comandos en tiempo real**, **soporte multi-región** y **configuración remota completa**.

### Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **GPS simulado** con múltiples modos
- **Sistema de estados** bien definido
- **Algoritmo Meshtastic integrado**
- **Modos de visualización flexibles**
- **Comandos en tiempo real** sin reinicio
- **Soporte multi-región LoRa** (US/EU/CH/AS/JP)
- **Configuración remota** entre dispositivos
- **Arquitectura completamente modular** para mantenibilidad

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
- **Responde a configuración remota**

### REPEATER
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos con **ROUTER priority**
- **SNR-based intelligent delays**
- **Responde a configuración remota**

### RECEIVER
- Recibe y visualiza posiciones GPS de toda la red
- **Duplicate detection y mesh statistics**
- **Puede configurar otros dispositivos remotamente**
- **Discovery de dispositivos en red**

---

## Estado Actual del Proyecto

### **TODAS LAS FASES COMPLETADAS**

**FASE 1: Sistema de Configuración Serial** - COMPLETADA  
**FASE 2: GPS Simulado** - COMPLETADA  
**FASE 3: LoRa + Mesh Completo** - COMPLETADA  
**FASE 4: Modos de Visualización** - COMPLETADA  
**FASE 5: Configuración Remota y Multi-Región** - COMPLETADA  

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

## Comandos Disponibles

### Comandos de Configuración

```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>  # Configurar rol del dispositivo
CONFIG_DEVICE_ID <1-999>                 # Configurar ID único
CONFIG_GPS_INTERVAL <5-3600>             # Intervalo GPS en segundos
CONFIG_MAX_HOPS <1-10>                   # Máximo saltos en mesh
CONFIG_DATA_MODE <SIMPLE|ADMIN>          # Modo de visualización
CONFIG_REGION <US|EU|CH|AS|JP>           # Región LoRa (frecuencia)
CONFIG_SAVE                              # Guardar configuración
CONFIG_RESET                             # Resetear configuración
INFO                                     # Información del dispositivo
STATUS                                   # Estado y configuración actual
START                                    # Iniciar modo operativo
HELP                                     # Mostrar todos los comandos
```

### Comandos Durante Operación

```bash
MODE SIMPLE                              # Cambiar a vista simple
MODE ADMIN                               # Cambiar a vista completa
```

### Comandos Específicos del RECEIVER

```bash
DISCOVER                                 # Buscar dispositivos en red
REMOTE_CONFIG <deviceID>                 # Configurar dispositivo remoto
```

### Comandos de Configuración Remota

**Dentro del modo `REMOTE_CONFIG <deviceID>`:**
```bash
REMOTE_GPS_INTERVAL <5-3600>             # Cambiar intervalo GPS remoto
REMOTE_DATA_MODE <SIMPLE|ADMIN>          # Cambiar modo datos remoto
REMOTE_STATUS                            # Obtener estado del dispositivo
REMOTE_REBOOT                            # Reiniciar dispositivo remoto
EXIT                                     # Salir de configuración remota
```

---

## Regiones LoRa Soportadas

| Región | Código | Frecuencia | Países/Zonas |
|--------|--------|------------|--------------|
| **US** | `US` | 915 MHz | Estados Unidos, México, Canadá |
| **EU** | `EU` | 868 MHz | Europa, África, Medio Oriente |
| **CH** | `CH` | 470 MHz | China |
| **AS** | `AS` | 433 MHz | Asia (general) |
| **JP** | `JP` | 920 MHz | Japón |

---

## Modos de Visualización

### Modo SIMPLE

**Propósito**: Vista limpia que muestra solo el packet básico según los key requirements del proyecto.

**Formato del packet**: `[deviceID, latitude, longitude, batteryvoltage, timestamp]`

**Ejemplo de salida**:
```
[001,25.302677,-98.277664,4100,1718661234]
Envío realizado
```

### Modo ADMIN

**Propósito**: Vista completa con toda la información de mesh, estadísticas y diagnóstico.

**Ejemplo de salida**:
```
[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Región: US (Estados Unidos/México)
Frecuencia: 915.0 MHz
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

---

## Arquitectura del Código (Completamente Modular)

### Estructura del Proyecto

```
src/
├── main.cpp
├── battery/
│   ├── battery_manager.h
│   └── battery_manager.cpp
├── config/
│   ├── config_manager.h
│   ├── config_manager.cpp
│   ├── config_commands.h
│   ├── config_commands.cpp
│   ├── config_help.h
│   └── config_help.cpp
├── gps/
│   ├── gps_manager.h
│   ├── gps_manager
│   ├── gps_simulation.h
│   ├── gps_simulation.cpp
│   ├── gps_utils.h
│   └── gps_utils.cpp
├── roles/
│   ├── role_manager.h
│   ├── role_manager.cpp         
│   ├── tracker_role.h
│   ├── tracker_role.cpp
│   ├── repeater_role.h
│   ├── repeater_role.cpp
│   ├── receiver_role.h
│   └── receiver_role.cpp
├── display/
│   ├── display_manager.h
│   ├── display_manager.cpp      
│   ├── simple_display.h
│   ├── simple_display.cpp
│   ├── admin_display.h
│   └── admin_display.cpp
├── serial/
│   ├── serial_handler.h
│   ├── serial_handler.cpp
│   ├── remote_commands.h
│   └── remote_commands.cpp
├── lora.h
└── lora/
    ├── lora_types.h
    ├── lora_hardware.h
    ├── lora_hardware.cpp
    ├── lora_manager.h
    ├── lora_manager.cpp
    ├── lora_comm.h
    ├── lora_comm.cpp
    ├── lora_packet.h
    ├── lora_packet.cpp
    ├── lora_mesh.h
    ├── lora_mesh.cpp
    ├── lora_remote_config.h
    └── lora_remote_config.cpp
```

---

## Módulos Principales

### **config/** - Sistema de Configuración
- **config_manager**: Core del sistema (load/save/begin)
- **config_commands**: Manejadores de comandos seriales
- **config_help**: Sistema de ayuda y documentación

### **gps/** - Sistema GPS
- **gps_manager**: Coordinador principal sin dependencias
- **gps_simulation**: Todos los modos de simulación
- **gps_utils**: Cálculos geográficos (Haversine, bearing)

### **battery/** - Monitoreo de Batería
- **battery_manager**: Sistema independiente extraído de GPS
- Simulación de descarga gradual
- Integración en packet format

### **lora/** - Sistema LoRa Modular
- **lora_hardware**: SX1262 + soporte multi-región
- **lora_manager**: Coordinación y utilidades
- **lora_comm**: TX/RX básica + mensajes remotos
- **lora_packet**: Procesamiento y validación
- **lora_mesh**: Algoritmo Meshtastic completo
- **lora_remote_config**: Discovery + comandos remotos

### **roles/** - Lógica por Roles
- **role_manager**: Coordinador de roles
- **tracker_role**: Lógica específica TRACKER
- **repeater_role**: Lógica específica REPEATER  
- **receiver_role**: Lógica específica RECEIVER

### **display/** - Visualización
- **display_manager**: Coordinador de modos
- **simple_display**: Modo SIMPLE
- **admin_display**: Modo ADMIN

### **serial/** - Comandos Seriales
- **serial_handler**: Router de comandos
- **remote_commands**: Comandos remotos para RECEIVER

---

## Flujo de Datos Modular

```
[main.cpp] 
    ↓
[role_manager] → Coordinación según rol configurado
    ↓
[tracker_role / repeater_role / receiver_role]
    ↓
[gps_manager] → [gps_simulation] + [gps_utils]
    ↓
[battery_manager] → Voltage independiente
    ↓
[lora_manager] → [lora_hardware] + [lora_comm] + [lora_mesh]
    ↓
[display_manager] → [simple_display / admin_display]
    ↓
[serial_handler] → [remote_commands] (solo RECEIVER)
```

---

## Packet Format y Battery Monitoring

### Formato Exacto (Según Key Requirements)

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

## Configuración Remota

### Discovery de Dispositivos

**Solo disponible en RECEIVER:**

```bash
DISCOVER
[INFO] Buscando dispositivos en la red...
[INFO] Discovery request enviado. Esperando respuestas...

[FOUND] Device 1: TRACKER (RSSI: -45dBm, GPS: 15s, Mode: SIMPLE, Region: US, Battery: 4100mV, Uptime: 3600s)
[FOUND] Device 2: REPEATER (RSSI: -52dBm, GPS: 30s, Mode: ADMIN, Region: US, Battery: 3950mV, Uptime: 7200s)

[INFO] Discovery completado.
```

### Configuración Remota de Dispositivos

**Solo disponible en RECEIVER:**

```bash
REMOTE_CONFIG 001
[INFO] Configurando dispositivo 1...
[INFO] Comandos: REMOTE_GPS_INTERVAL, REMOTE_DATA_MODE, REMOTE_STATUS, REMOTE_REBOOT, EXIT

remote_001> REMOTE_GPS_INTERVAL 20
[OK] Enviando comando a device 1...
[INFO] Comando enviado. Esperando respuesta...
[RESPONSE] Device 1: GPS interval cambiado a 20s

remote_001> REMOTE_DATA_MODE ADMIN
[OK] Enviando comando a device 1...
[RESPONSE] Device 1: Modo cambiado a ADMIN

remote_001> REMOTE_STATUS
[OK] Enviando comando a device 1...
[RESPONSE] Device 1: 1,GPS:20s,Bat:4100mV

remote_001> EXIT
[INFO] Saliendo de configuración remota
```

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

## Guía de Uso

### Configuración de un TRACKER (con región)

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

config> CONFIG_REGION US
[OK] Región configurada: US (Estados Unidos/México)
[INFO] Frecuencia: 915.0 MHz

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

config> START
[OK] Iniciando modo operativo...

# Sistema operando en modo SIMPLE
[001,25.302677,-98.277664,4100,1718661234]
Envío realizado

# Cambio a modo ADMIN en tiempo real
MODE ADMIN
[OK] Modo cambiado a: ADMIN
[INFO] Mostrando información completa
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

**"Comando CONFIG_REGION no reconocido"**
```bash
# Verificar versión del firmware
1. Comando correcto: CONFIG_REGION <US|EU|CH|AS|JP>
2. Reiniciar después de CONFIG_SAVE para aplicar nueva frecuencia
```

**"Comando REMOTE_CONFIG no funciona"**
```bash
# Solo disponible en RECEIVER:
1. Verificar que el dispositivo esté configurado como RECEIVER
2. Comando correcto: REMOTE_CONFIG <deviceID>
3. Usar DISCOVER primero para encontrar dispositivos
4. Asegurar que los dispositivos estén en la misma región LoRa
```

### Problemas de LoRa Multi-Región

**"LoRa no se conecta después de cambiar región"**
```bash
# Verificar configuración de región:
1. Reiniciar dispositivo después de CONFIG_SAVE
2. Verificar que todos los dispositivos usen la misma región
3. Comando STATUS para confirmar frecuencia actual
4. Verificar regulaciones locales de la frecuencia seleccionada
```

---

## Estado de Cumplimiento de Requirements

### Key Requirements COMPLETADOS Y SUPERADOS

| Requirement | Estado | Notas |
|-------------|--------|-------|
| **Fork Meshtastic y simplificar** | **SUPERADO** | Implementado desde cero con arquitectura modular |
| **Compatibilidad ESP32-S3** | Completado | XIAO ESP32S3 específicamente |
| **Easy to flash** | Completado | PlatformIO setup sencillo |
| **Remove unnecessary features** | Completado | Solo funciones esenciales |
| **Lightweight packet format** | **SUPERADO** | **[deviceID,lat,lon,battery,timestamp]** |
| **Mesh rebroadcast** | **SUPERADO** | **Algoritmo completo Meshtastic** |
| **Battery monitoring** | **AGREGADO** | **Voltage en cada packet** |
| **EEPROM storage** | **SUPERADO** | Device ID, GPS interval, data mode, **región** |
| **Remote configuration** | **AGREGADO** | **Sistema completo no solicitado** |

### System Behavior COMPLETADOS Y SUPERADOS

| Behavior | Estado | Notas |
|----------|--------|-------|
| **Fully off-grid** | Completado | Sin WiFi/cellular/internet |
| **Configurable GPS intervals** | **SUPERADO** | 5-3600 segundos + **config remota** |
| **Autonomous trackers** | Completado | Operación independiente |
| **Real mesh testing** | **FUNCIONANDO** | **Demo exitosa realizada** |
| **Multi-region support** | **AGREGADO** | **US/EU/CH/AS/JP frequencies** |

### System Features COMPLETADAS

| Feature | Estado | Progreso |
|---------|--------|----------|
| **Data visualization modes** | **COMPLETADO** | **SIMPLE + ADMIN modes** |
| **Real-time mode switching** | **COMPLETADO** | **MODE commands** |
| **Battery monitoring** | **COMPLETADO** | **Integrated in packets** |
| **Auto-save configuration** | **COMPLETADO** | **EEPROM persistence** |
| **Multi-region LoRa** | **COMPLETADO** | **5 regiones soportadas** |
| **Remote configuration** | **COMPLETADO** | **Discovery + Remote commands** |
| **Modular architecture** | **COMPLETADO** | **33 archivos especializados** |

---

## Performance y Capacidad de la Red

### Estimación Teórica de Nodos

Basado en el algoritmo de Meshtastic y las limitaciones del protocolo:

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| **Nodos máximos teóricos** | ~100-150 | Basado en duplicate detection table |
| **Nodos prácticos recomendados** | 20-30 | Para performance óptima |
| **Máximo saltos** | 3 | Configurado como Meshtastic estándar |
| **Tiempo de vida de packet** | 5 minutos | Memory cleanup en duplicate detection |
| **Intervalo GPS mínimo** | 5 segundos | Balance entre updates y congestión |

### Factores Limitantes

1. **Memory de Duplicate Detection**: 100 packets máximo en memoria
2. **Contention Window**: Delays aumentan con más nodos
3. **Airtime Regulations**: Limitaciones legales por región
4. **Battery Life**: Más retransmisiones = mayor consumo
5. **Processing Power**: ESP32-S3 limits para packets/segundo

### Optimizaciones Implementadas

- **SNR-based delays**: Nodos lejanos retransmiten primero
- **Role priority**: REPEATERS tienen prioridad sobre CLIENTS
- **Hop limiting**: Máximo 3 saltos para evitar loops
- **Duplicate filtering**: Previene retransmisiones innecesarias
- **Efficient packet format**: Solo 16 bytes de payload GPS

---

## Documentación Adicional

### Recursos Útiles
- [Documentación de Meshtastic](https://meshtastic.org/) - Referencia del protocolo original
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)
- [LoRa Regional Parameters](https://lora-alliance.org/resource_hub/rp2-1-0-3-lorawan-regional-parameters/)

### Algoritmo Meshtastic
- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Equipo

### Desarrollo Principal
- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Engineer**: Bryan Caleb Martinez Cavazos

---

## Changelog

### Versión 0.4 (Actual)
- **Proyecto modularizado 100%**

### Versión 0.3
- **Sistema Multi-Región LoRa** - Soporte US/EU/CH/AS/JP
- **Configuración Remota Completa** - Discovery + Remote commands
- **Arquitectura Modular** - Código LoRa reorganizado
- **Comandos Remotos** - GPS_INTERVAL, DATA_MODE, STATUS, REBOOT
- **Mensajes de Discovery** - Request/Response system
- **Mejoras en Error Handling** - Mejor gestión de errores
- **Optimizaciones de Performance** - Cleanup automático de memoria

### Versión 0.2.1
- **Modos de Visualización** - SIMPLE y ADMIN
- **Comandos en Tiempo Real** - MODE SIMPLE/ADMIN durante operación
- **Battery Monitoring** - Voltage integrado en packets

### Versión 0.2
- **Algoritmo Meshtastic** - Duplicate detection, SNR delays
- **Mesh Statistics** - Tracking completo de performance
- **Role-based Priority** - REPEATER vs CLIENT priorities
- **Hop Management** - Control de TTL con máximo 3 saltos

### Versión 0.1
- **Sistema de Configuración** - Comandos seriales robustos
- **GPS Simulado** - 5 modos de simulación
- **LoRa Básico** - Comunicación punto a punto
- **Persistencia EEPROM** - Configuración no volátil

---

**Última actualización**: Julio 1, 2025  
**Versión del firmware**: 0.4