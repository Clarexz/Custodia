# Custom LoRa Mesh Asset Tracking System

**Firmware personalizado basado en Meshtastic para tracking GPS off-grid**

![Estado del Proyecto](https://img.shields.io/badge/Estado-Fase%202%20Completada-green)
![Versión](https://img.shields.io/badge/Versión-1.0.0-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)

---

## Descripción del Proyecto

Sistema de tracking GPS completamente off-grid usando una red mesh LoRa. Implementación desde cero optimizada específicamente para asset tracking, eliminando la complejidad del Meshtastic original.

### Objetivo Principal
Crear un firmware unificado que permita configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa, con configuración completa via comandos seriales y operación autónoma.

### Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **GPS simulado realista** con múltiples modos
- **Sistema de estados** bien definido

---

## Hardware Soportado

| Componente | Modelo | Estado | Función |
|------------|--------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Implementado | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | Próxima fase | Comunicación mesh de largo alcance |
| **Pantalla** | ESP32-compatible TFT | Futuro | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Simulado | Geolocalización |

---

## Roles del Sistema

### TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- Optimizado para bajo consumo energético
- **Estado actual**: Completamente funcional con GPS simulado

### REPEATER  
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos
- No transmite su propia posición
- **Estado actual**: Lógica básica implementada

### RECEIVER
- Recibe y visualiza posiciones GPS
- Incluye pantalla para mapas offline
- Puede configurar otros dispositivos remotamente
- **Estado actual**: Implementado, pendiente mapas offline

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

### **Próximas Fases**

| Fase | Descripción | Estado | Prioridad |
|------|-------------|--------|-----------|
| **Fase 3** | LoRa básico - Comunicación real | En progreso | Crítico |
| **Fase 4** | Mesh routing simplificado | Planificado | Crítico |
| **Fase 5** | Características avanzadas | Planificado | Importante |

---

## Instalación y Configuración

### Prerrequisitos
- **Visual Studio Code** con extensión PlatformIO
- **XIAO ESP32S3** microcontrolador
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

### Configuración de Hardware para Próximas Fases

**LoRa Setup (Fase 3):**

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

config> CONFIG_GPS_INTERVAL 30
[OK] Intervalo GPS configurado: 30 segundos

config> STATUS
=== CONFIGURACIÓN ACTUAL ===
Rol: TRACKER
Device ID: 1
Intervalo GPS: 30 segundos
Máximo saltos: 3
============================

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

config> START
[OK] Iniciando modo operativo...
```

### Salida Esperada en Modo TRACKER

```
[TRACKER] === TRANSMISIÓN GPS ===
Device ID: 1
Coordenadas: 25.302677,-98.277664
Timestamp: 1718661234
Packet: 1,25.302677,-98.277664,1718661234
Próxima transmisión en 30 segundos
==============================
```

### Configuración de otros Roles

**REPEATER:**
```bash
CONFIG_ROLE REPEATER
CONFIG_DEVICE_ID 002
CONFIG_SAVE
START
```

**RECEIVER:**
```bash
CONFIG_ROLE RECEIVER
CONFIG_DEVICE_ID 003
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
└── gps.cpp          # Sistema GPS - Implementación

platformio.ini        # Configuración del proyecto PlatformIO
README.md             # Esta documentación
```

### Flujo de Datos

```
[Startup] → [Config Manager] → [EEPROM Load] → [Estado Inicial]
                                    ↓
[CONFIG_MODE] ←→ [Comandos Seriales] ←→ [Validación]
      ↓                                      ↓
[CONFIG_SAVE] → [EEPROM] → [START] → [RUNNING]
                                        ↓
                              [GPS Manager] → [Simulación]
                                        ↓
                              [Packet Format] → [Transmisión]
```

### Módulos Principales

#### **ConfigManager** (`config.h/cpp`)
**Responsabilidades:**
- Procesamiento de comandos seriales
- Persistencia en EEPROM
- Validación de parámetros
- Control de estados del sistema

**Métodos clave:**
- `begin()` - Inicialización del sistema
- `processSerialInput()` - Parser de comandos
- `saveConfig()` / `loadConfig()` - Persistencia
- `handleConfig*()` - Manejadores específicos de comandos

#### **GPSManager** (`gps.h/cpp`)
**Responsabilidades:**
- Simulación GPS realista
- Cálculos geográficos (Haversine)
- Formateo de datos para transmisión
- Diferentes modos de movimiento

**Métodos clave:**
- `begin(mode)` - Inicialización con modo específico
- `update()` - Actualización continua de coordenadas
- `getCurrentData()` - Obtener datos GPS actuales
- `formatForTransmission()` - Generar packet listo para envío

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

**Upload exitoso pero no hay output en el monitor**
```bash
# Solución: Reset manual
1. Presionar botón RESET (R) una vez
2. El programa debería iniciar y mostrar mensajes
```

### Problemas de Configuración

**"Sin fix GPS - Esperando señal..."**
```bash
# Solución: Reset del sistema
1. Presionar botón RESET (R)
2. O enviar cualquier comando para volver a CONFIG_MODE
3. Luego START nuevamente
```

**Configuración no se guarda**
```bash
# Verificar secuencia correcta:
1. CONFIG_ROLE y CONFIG_DEVICE_ID son obligatorios
2. Ejecutar CONFIG_SAVE después de configurar
3. Verificar con STATUS antes de START
```

### Problemas del Monitor Serial

**No veo lo que escribo**
- **Normal en PlatformIO** - comandos aparecen al presionar Enter
- **Alternativa**: Usar Arduino IDE Serial Monitor

**Monitor no responde**
- Verificar velocidad: 115200 baud
- Reconectar USB si es necesario

---

## Roadmap del Proyecto

### Fase 3: LoRa Básico (En progreso)
- [ ] Integración RadioLib para SX1262
- [ ] Transmisión punto a punto básica
- [ ] Protocolo de packets binario
- [ ] Pruebas de alcance y confiabilidad

### Fase 4: Mesh Simplificado
- [ ] Algoritmo de routing básico
- [ ] Manejo de colisiones con random delay
- [ ] Implementación de TTL (Time To Live)
- [ ] Estadísticas de red y nodos

### Fase 5: Características Avanzadas
- [ ] Remote configuration (receiver-to-tracker)
- [ ] Battery monitoring y voltage reporting
- [ ] Modo sleep inteligente
- [ ] Mapas offline en RECEIVER
- [ ] Estimación teórica de capacidad de nodos

---

## Documentación Adicional

### Recursos Útiles
- [Documentación de Meshtastic](https://meshtastic.org/) - Referencia del protocolo original
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [Seed Studio XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

---

## Estado de Cumplimiento de Requirements

### Key Requirements

| Requirement | Estado | Notas |
|-------------|--------|-------|
| **Fork Meshtastic y simplificar** | Implementado desde cero | Approach alternativo más eficiente |
| **Compatibilidad ESP32-S3** | Completado | XIAO ESP32S3 específicamente |
| **Easy to flash** | Completado | PlatformIO setup sencillo |
| **Remote configuration** | En progreso | Será parte de Fase 3 (LoRa) |
| **GitHub documentation** | En progreso | Este README + docs técnicos faltantes |
| **Remove unnecessary features** | Completado | Solo funciones esenciales |
| **Lightweight packet format** | Completado | `deviceID,lat,lon,timestamp` |
| **Mesh rebroadcast** | Planificado | Fase 4 - Random delay incluido |
| **Battery monitoring** | Planificado | Fase 5 |
| **EEPROM storage** | Completado | Device ID y GPS interval |
| **Theoretical mesh estimation** | Planificado | Cálculo basado en Meshtastic |

### System Behavior

| Behavior | Estado | Notas |
|----------|--------|-------|
| **Fully off-grid** | Completado | Sin WiFi/cellular/internet |
| **Configurable GPS intervals** | Completado | 5-3600 segundos |
| **Autonomous trackers** | Completado | Operación independiente |
| **Offline maps visualization** | Planificado | Fase 5 - RECEIVER |
| **Real mesh testing** | Próximo | Fase 3 - LoRa hardware |

### Documentation

| Document | Estado | Notas |
|----------|--------|-------|
| **User manual** | En progreso | Basado en este README |
| **Extended technical** | En progreso | Doc técnico empezado con el avance actual |
| **Setup procedures** | Completado | Sección de instalación |
| **Packet format details** | Completado | Documentado y implementado |
| **Troubleshooting** | Completado | Sección troubleshooting |
| **Clean, commented code** | Completado | Código documentado |

---

## Equipo

- **Product Manager/Engineer**: Gilberto Castro Sustaita
- **Engineer**: Bryan Caleb Martinez Cabazos

---

**Última actualización**: Junio 17, 2025  
**Versión del firmware**: 1.0.0  
**Estado del proyecto**: FASE 2 COMPLETADA - FASE 3 EN PROGRESO