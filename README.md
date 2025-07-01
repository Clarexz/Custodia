# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-Fase%205%20COMPLETADA-brightgreen)
![Versión](https://img.shields.io/badge/Versión-0.3-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Algoritmo](https://img.shields.io/badge/Algoritmo-Meshtastic%20Completo-purple)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa con **algoritmo completo de Meshtastic**. Implementación que integra el sistema de Managed Flood Routing con **modos de visualización flexibles**, **comandos en tiempo real**, **soporte multi-región** y **configuración remota**.

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

---

## Hardware Soportado

| Componente | Modelo | Estado | Función |
|------------|--------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Implementado | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | MESH FUNCIONAL | Comunicación mesh de largo alcance |
| **Pantalla** | ESP32-compatible TFT | Futuro | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Simulado | Geolocalización |

---

## Roles del Sistema

### TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- Mesh routing con CLIENT priority
- Responde a configuración remota

### REPEATER
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos con ROUTER priority
- SNR-based intelligent delays
- Responde a configuración remota

### RECEIVER
- Recibe y visualiza posiciones GPS de toda la red
- Duplicate detection y mesh statistics
- Puede configurar otros dispositivos remotamente
- Discovery de dispositivos en red

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
- [x] **Modo SIMPLE** - Vista limpia con solo packet format según key requirements
- [x] **Modo ADMIN** - Vista completa con estadísticas mesh y diagnóstico
- [x] **Comando CONFIG_DATA_MODE** - Configuración durante setup
- [x] **Comandos MODE en tiempo real** - Cambio sin reinicio durante operación
- [x] **Auto-guardado automático** - Cambios persisten en EEPROM
- [x] **Battery monitoring** - Voltage de batería en cada packet
- [x] **Soporte todos los roles** - TRACKER, REPEATER, RECEIVER

### **FASE 5: Configuración Remota y Multi-Región** - COMPLETADA

**Funcionalidades implementadas:**
- [x] **Sistema Multi-Región LoRa** - US (915MHz), EU (868MHz), CH (470MHz), AS (433MHz), JP (920MHz)
- [x] **Comando CONFIG_REGION** - Configuración de región durante setup
- [x] **Discovery de Dispositivos** - Comando DISCOVER para encontrar dispositivos activos
- [x] **Configuración Remota Completa** - RECEIVER puede configurar otros dispositivos
- [x] **Comandos Remotos** - REMOTE_GPS_INTERVAL, REMOTE_DATA_MODE, REMOTE_STATUS, REMOTE_REBOOT
- [x] **Mensajes de Configuración** - MSG_DISCOVERY_REQUEST/RESPONSE, MSG_CONFIG_CMD/RESPONSE
- [x] **Arquitectura Modular** - Código LoRa separado en módulos especializados

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

### Configuración de Región

```bash
# Durante configuración inicial
config> CONFIG_REGION US
[OK] Región configurada: US (Estados Unidos/México)
[INFO] Frecuencia: 915.0 MHz
[WARNING] Reinicie el dispositivo después de CONFIG_SAVE para aplicar nueva frecuencia

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

# El dispositivo usará 915 MHz después del reinicio
```

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

## Guía de Uso Completa

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

config> STATUS
=== CONFIGURACIÓN ACTUAL ===
Rol: TRACKER
Device ID: 1
Intervalo GPS: 15 segundos
Máximo saltos: 3
Modo de datos: SIMPLE
Región LoRa: US (915.0 MHz)
============================

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

[TRACKER] === TRANSMISIÓN GPS + LoRa MESH ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Región: US (Estados Unidos/México)
Frecuencia: 915.0 MHz
...
```

### Configuración de un RECEIVER (con capacidades remotas)

```bash
config> CONFIG_ROLE RECEIVER
[OK] Rol configurado: RECEIVER

config> CONFIG_DEVICE_ID 003
[OK] Device ID configurado: 3

config> CONFIG_REGION US
[OK] Región configurada: US (Estados Unidos/México)

config> CONFIG_DATA_MODE ADMIN
[OK] Modo de datos configurado: ADMIN

config> CONFIG_SAVE
config> START

# Una vez operativo, comandos especiales disponibles:
DISCOVER
REMOTE_CONFIG 001
HELP
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

## Arquitectura del Código

### Estructura del Proyecto Modular

```
src/
├── main.cpp                 # Loop principal y lógica de estados
├── config.h                # Sistema de configuración - Declaraciones
├── config.cpp              # Sistema de configuración - Implementación
├── gps.h                   # Sistema GPS - Declaraciones
├── gps.cpp                 # Sistema GPS - Implementación + Battery
├── lora.h                  # Sistema LoRa + Mesh - Declaraciones principales
|-- lora/
      ├── lora_packet.cpp         # Procesamiento y validación de packets
      ├── lora_comm.cpp           # Transmisión y recepción básica
      ├── lora_hardware.cpp       # Inicialización hardware SX1262 + Multi-región
      ├── lora_manager.cpp        # Coordinación y utilidades
      ├── lora_mesh.cpp           # Algoritmo Meshtastic completo
      └── lora_remote_config.cpp  # Configuración remota y discovery

platformio.ini              # Configuración del proyecto PlatformIO
README.md                   # Esta documentación
```

### Flujo de Datos Enhanced con Configuración Remota

```
[Startup] → [Config Manager] → [EEPROM Load] → [Estado Inicial]
                                    ↓
[CONFIG_MODE] ←→ [Comandos Seriales] ←→ [Validación + Región]
      ↓                                      ↓
[CONFIG_SAVE] → [EEPROM] → [START] → [RUNNING]
                                        ↓
                              [GPS Manager] → [Simulación + Battery]
                                        ↓
                              [LoRa Manager] → [Meshtastic + Multi-región]
                                        ↓
                     [Duplicate Detection] → [SNR Delays] → [Mesh Routing]
                                        ↓
                              [Remote Config] → [Discovery + Commands]
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
- **Gestión de regiones LoRa**
- **Gestión de modos de visualización**
- **Comandos en tiempo real**

#### **GPSManager** (`gps.h/cpp`)
**Responsabilidades:**
- Simulación GPS realista
- Cálculos geográficos (Haversine)
- Formateo de datos para transmisión
- Diferentes modos de movimiento
- Battery monitoring integrado

#### **LoRaManager** (Modular - 6 archivos)
**Responsabilidades principales:**

**`lora.h`** - Declaraciones principales y estructuras
**`lora_hardware.cpp`** - Hardware SX1262 + soporte multi-región
**`lora_comm.cpp`** - Transmisión y recepción básica
**`lora_packet.cpp`** - Procesamiento y validación de packets
**`lora_mesh.cpp`** - Algoritmo completo de Meshtastic
**`lora_remote_config.cpp`** - Configuración remota y discovery
**`lora_manager.cpp`** - Coordinación y utilidades

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

## Testing y Validación

### Tests Realizados

#### **Comunicación Básica** 
- [x] Transmisión TRACKER → RECEIVER
- [x] Retransmisión via REPEATER
- [x] Duplicate detection funcionando
- [x] SNR-based delays operativos

#### **Configuración Remota**
- [x] Discovery de dispositivos
- [x] Comandos remotos (GPS_INTERVAL, DATA_MODE)
- [x] Respuestas y confirmaciones
- [x] Error handling

#### **Multi-Región**
- [x] Configuración de frecuencias por región
- [x] Persistencia en EEPROM
- [x] Aplicación después de reinicio

---

## Seguridad y Consideraciones

### Aspectos de Seguridad

**Características Actuales:**
- **Sin encryption**: Datos transmitidos en claro (por simplicidad)
- **Device ID único**: Previene colisiones básicas
- **Duplicate detection**: Previene replay attacks accidentales

**Consideraciones de Seguridad:**
- Todos los datos GPS son visibles en la red
- Configuración remota sin autenticación
- Posible interferencia o jamming

**Recomendaciones para Producción:**
- Implementar encryption básico (AES)
- Autenticación para comandos remotos
- Rate limiting para prevenir spam
- Whitelist de device IDs permitidos

---

## Equipo

### Desarrollo Principal
- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Engineer**: Bryan Caleb Martinez Cavazos

---

## Changelog

### Versión 0.3 (Actual)
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
**Versión del firmware**: 0.3