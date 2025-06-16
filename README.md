# Custom LoRa Mesh Asset Tracking System
## 📋 Descripción del Proyecto

Este proyecto desarrolla un sistema de tracking GPS completamente off-grid usando una red mesh LoRa. Está basado en una versión simplificada y personalizada del firmware Meshtastic, optimizada específicamente para asset tracking.

### 🎯 Objetivo Principal
Crear un firmware unificado que permita configurar dispositivos ESP32-S3 para diferentes roles en una red mesh LoRa, eliminando la complejidad innecesaria del Meshtastic original.

### 🏗️ Arquitectura del Sistema
- **Un solo firmware** para todos los dispositivos
- **Configuración por roles** via comandos seriales
- **Persistencia en EEPROM** para configuraciones
- **Sistema de estados** bien definido

---

## 🔧 Hardware Soportado

| Componente | Modelo | Función |
|------------|--------|---------|
| **Microcontrolador** | XIAO ESP32S3 | Control principal y procesamiento |
| **Módulo LoRa** | Wio SX1262 | Comunicación mesh de largo alcance |
| **Pantalla** | ESP32-compatible TFT | Visualización (solo RECEIVER) |
| **GPS** | Módulo compatible UART | Geolocalización |

---

## 📡 Roles del Sistema

### 🎯 TRACKER
- Transmite su posición GPS periódicamente
- Opera autónomamente sin interfaz de usuario
- Optimizado para bajo consumo energético

### 🔄 REPEATER  
- Actúa como extensor de rango
- Retransmite mensajes de otros dispositivos
- No transmite su propia posición

### 📺 RECEIVER
- Recibe y visualiza posiciones GPS
- Incluye pantalla para mapas offline
- Puede configurar otros dispositivos remotamente

---

## 🚀 Estado Actual: FASE 1 COMPLETADA

### ✅ Características Implementadas

- [x] **Sistema de configuración serial completo**
- [x] **Persistencia en EEPROM** usando Preferences
- [x] **Validación de parámetros** con rangos específicos
- [x] **Control de estados del sistema** (BOOT, CONFIG_MODE, RUNNING, SLEEP)
- [x] **Indicadores visuales LED** según el modo
- [x] **Manejo de errores robusto** con confirmaciones
- [x] **Documentación integrada** (comando HELP)
- [x] **Arquitectura modular** preparada para expansión

### 🔄 Próximas Fases

| Fase | Descripción | Estado |
|------|-------------|--------|
| **Fase 2** | Funcionalidad GPS básica | 🚧 Siguiente |
| **Fase 3** | Comunicación LoRa punto a punto | ⏳ Planificado |
| **Fase 4** | Protocolo mesh simplificado | ⏳ Planificado |
| **Fase 5** | Características avanzadas | ⏳ Planificado |

---

## 💻 Instalación y Configuración

### Prerrequisitos
- **Visual Studio Code** con extensión PlatformIO
- **ESP32-S3** con módulo LoRa SX1262
- **Cable USB-C** para programación

### Pasos de Instalación

1. **Clonar el repositorio**
   ```bash
   git clone https://github.com/[usuario]/custom-meshtastic-tracker.git
   cd custom-meshtastic-tracker
   ```

2. **Abrir en PlatformIO**
   - Abrir VS Code
   - File → Open Folder → Seleccionar carpeta del proyecto

3. **Compilar y subir**
   - Conectar ESP32-S3 via USB-C
   - Click en "Build" en PlatformIO
   - Click en "Upload"
   - Si hay problemas: mantener BOOT + presionar RESET + soltar BOOT + Upload inmediato

4. **Verificar funcionamiento**
   - Click en "Monitor" (115200 baud)
   - Debería aparecer el mensaje de bienvenida

---

## 🎮 Comandos Disponibles

### Configuración Básica
```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>  # Configurar rol del dispositivo
CONFIG_DEVICE_ID <1-999>                 # Configurar ID único
CONFIG_SAVE                              # Guardar configuración
START                                    # Iniciar modo operativo
```

### Configuración Avanzada
```bash
CONFIG_GPS_INTERVAL <5-3600>             # Intervalo GPS en segundos
CONFIG_MAX_HOPS <1-10>                   # Máximo saltos en mesh
CONFIG_RESET                             # Resetear configuración
```

### Información y Ayuda
```bash
INFO                                     # Información del dispositivo
STATUS                                   # Estado y configuración actual
HELP                                     # Mostrar todos los comandos
```

### Ejemplo de Configuración Completa
```bash
CONFIG_ROLE TRACKER
CONFIG_DEVICE_ID 001
CONFIG_GPS_INTERVAL 30
CONFIG_SAVE
START
```

---

## 📁 Estructura del Proyecto

```
src/
├── main.cpp          # Loop principal y lógica de estados
├── config.h          # Definiciones del sistema de configuración
└── config.cpp        # Implementación del sistema de configuración

platformio.ini        # Configuración del proyecto PlatformIO
README.md             # Esta documentación
```

### Descripción de Archivos Principales

#### `src/config.h`
- **Enumeraciones** para roles y estados del sistema
- **Estructura DeviceConfig** para almacenar configuración
- **Clase ConfigManager** con interfaz completa de comandos
- **Declaraciones** de métodos públicos y privados

#### `src/config.cpp`
- **Implementación completa** del sistema de configuración
- **Parser de comandos seriales** con validación
- **Persistencia en EEPROM** usando Preferences
- **Manejo de errores** y confirmaciones de usuario

#### `src/main.cpp`
- **Inicialización del sistema** y hardware
- **Loop principal** con procesamiento de estados
- **Lógica básica** para cada rol del dispositivo
- **Indicadores visuales LED** según el modo

---

## 🔍 Ejemplos de Uso

### Configurar un Tracker
```bash
# 1. Conectar dispositivo y abrir monitor serial
# 2. El sistema iniciará en modo configuración
config> CONFIG_ROLE TRACKER
[OK] Rol configurado: TRACKER

config> CONFIG_DEVICE_ID 001
[OK] Device ID configurado: 1

config> CONFIG_GPS_INTERVAL 60
[OK] Intervalo GPS configurado: 60 segundos

config> CONFIG_SAVE
[OK] Configuración guardada exitosamente.

config> START
[OK] Iniciando modo operativo...
[TRACKER] Simulando transmisión GPS cada 60s
```

### Verificar Estado del Sistema
```bash
config> STATUS
=== STATUS DEL SISTEMA ===
Estado: MODO CONFIGURACIÓN
Configuración válida: SÍ

=== CONFIGURACIÓN ACTUAL ===
Rol: TRACKER
Device ID: 1
Intervalo GPS: 60 segundos
Máximo saltos: 3
============================
```

---

## 