@echo off
setlocal enabledelayedexpansion

REM ===================================================================
REM Custom LoRa Mesh Asset Tracking System - Windows Flash Tool
REM Auto-detect ESP32S3 and flash firmware with configuration
REM ===================================================================

echo.
echo ===============================================
echo    Custom LoRa Mesh Flash Tool (Windows)
echo ===============================================
echo.

REM Configuración por defecto
set "ROLE=TRACKER"
set "DEVICE_ID=1"
set "INTERVAL=15"
set "REGION=US"
set "MODE=SIMPLE"
set "RADIO=DESERT_LONG_FAST"
set "HOPS=3"
set "CHANNEL=TEAM_ALPHA"
set "PASSWORD="

REM Procesar argumentos de línea de comandos
:parse_args
if "%~1"=="" goto check_params
if "%~1"=="-role" (
    set "ROLE=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-id" (
    set "DEVICE_ID=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-interval" (
    set "INTERVAL=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-region" (
    set "REGION=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-mode" (
    set "MODE=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-radio" (
    set "RADIO=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-hops" (
    set "HOPS=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-channel" (
    set "CHANNEL=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-password" (
    set "PASSWORD=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help
echo Parámetro desconocido: %~1
goto show_help

:show_help
echo.
echo Uso: flash_esp32.bat [opciones]
echo.
echo Opciones:
echo   -role ROLE        Función del dispositivo (TRACKER/REPEATER/RECEIVER)
echo   -id ID            ID único del dispositivo (1-255)
echo   -interval SEC     Intervalo de transmisión GPS en segundos (5-3600)
echo   -region REGION    Región LoRa (US/EU/CH/AS/JP)
echo   -mode MODE        Modo de verbosidad (SIMPLE/ADMIN)
echo   -radio PROFILE    Perfil de radio (DESERT_LONG_FAST/MOUNTAIN_STABLE/URBAN_DENSE/MESH_MAX_NODES)
echo   -hops HOPS        Máximo saltos en mesh (1-10)
echo   -channel NAME     Nombre de red privada (3-20 caracteres alfanuméricos)
echo   -password PASS    Contraseña de red (8-32 caracteres, opcional)
echo   -h, --help        Mostrar esta ayuda
echo.
echo Ejemplos:
echo   flash_esp32.bat -role TRACKER -id 1 -channel TEAM_ALPHA
echo   flash_esp32.bat -role REPEATER -id 2 -interval 30 -region EU
echo.
exit /b 0

:check_params
REM Generar password automáticamente si no se proporcionó
if "%PASSWORD%"=="" (
    echo Generando password automáticamente...
    set "PASSWORD=AUTO%random:~-4%"
)

echo Configuración:
echo   Rol: %ROLE%
echo   ID: %DEVICE_ID%
echo   Intervalo: %INTERVAL% segundos
echo   Región: %REGION%
echo   Modo: %MODE%
echo   Radio: %RADIO%
echo   Saltos: %HOPS%
echo   Canal: %CHANNEL%
echo   Password: %PASSWORD%
echo.

REM Verificar que Python esté instalado
echo [1/5] Verificando Python...
py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python no encontrado. 
    echo Instala Python desde: https://www.python.org/downloads/
    echo Asegúrate de marcar "Add Python to PATH" durante la instalación
    pause
    exit /b 1
)
echo ✓ Python encontrado

REM Instalar PlatformIO si no está instalado
echo.
echo [2/5] Verificando PlatformIO...
py -m platformio --version >nul 2>&1
if %errorlevel% neq 0 (
    echo PlatformIO no encontrado. Instalando...
    py -m pip install platformio
    if %errorlevel% neq 0 (
        echo ERROR: No se pudo instalar PlatformIO
        pause
        exit /b 1
    )
    echo ✓ PlatformIO instalado correctamente
) else (
    echo ✓ PlatformIO ya está instalado
)

REM Detectar puerto ESP32
echo.
echo [3/5] Detectando ESP32-S3...
set "ESP_PORT="

REM Buscar puertos COM activos
for /f "tokens=1" %%i in ('py -c "import serial.tools.list_ports; ports = serial.tools.list_ports.comports(); [print(p.device) for p in ports if 'USB' in p.description.upper() or 'SERIAL' in p.description.upper()]" 2^>nul') do (
    set "ESP_PORT=%%i"
    goto port_found
)

:port_found
if "%ESP_PORT%"=="" (
    echo ERROR: No se detectó ningún ESP32-S3
    echo.
    echo Solución:
    echo 1. Conecta el ESP32-S3 via USB-C
    echo 2. Instala los drivers si es necesario
    echo 3. Verifica que el dispositivo aparezca en el Administrador de dispositivos
    pause
    exit /b 1
)

echo ✓ ESP32-S3 detectado en puerto: %ESP_PORT%

REM Compilar y flashear firmware
echo.
echo [4/5] Compilando y flasheando firmware...
echo Por favor espera, esto puede tomar varios minutos...

py -m platformio run
if %errorlevel% neq 0 (
    echo ERROR: Falló la compilación
    echo.
    echo Solución manual:
    echo 1. Mantener presionado BOOT en el ESP32
    echo 2. Presionar y soltar RESET
    echo 3. Soltar BOOT
    echo 4. Ejecutar: py -m platformio run --target upload
    pause
    exit /b 1
)

py -m platformio run --target upload
if %errorlevel% neq 0 (
    echo ERROR: Falló el flasheo
    echo.
    echo Solución manual:
    echo 1. Mantener presionado BOOT en el ESP32
    echo 2. Presionar y soltar RESET
    echo 3. Soltar BOOT
    echo 4. Ejecutar: py -m platformio run --target upload
    pause
    exit /b 1
)

echo ✓ Firmware flasheado correctamente

REM Configurar dispositivo via serial
echo.
echo [5/5] Configurando dispositivo...
echo Esperando que el dispositivo reinicie...
timeout /t 8 /nobreak >nul

REM Crear comandos de configuración
set "NETWORK_CMD=CONFIG_NETWORK %CHANNEL% %PASSWORD%"
set "CONFIG_CMD=CONFIG_BASIC %ROLE% %DEVICE_ID% %INTERVAL% %REGION% %MODE% %RADIO% %HOPS%"

echo Enviando configuración...
echo %NETWORK_CMD% > temp_config.txt
echo %CONFIG_CMD% >> temp_config.txt
echo CONFIG_SAVE >> temp_config.txt
echo START >> temp_config.txt

REM Enviar configuración via Python script temporal
py -c "
import serial
import time
import sys

try:
    ser = serial.Serial('%ESP_PORT%', 115200, timeout=5)
    time.sleep(2)
    
    commands = [
        '%NETWORK_CMD%',
        '%CONFIG_CMD%',
        'CONFIG_SAVE',
        'START'
    ]
    
    for cmd in commands:
        print(f'Enviando: {cmd}')
        ser.write((cmd + '\n').encode())
        time.sleep(1)
        
    ser.close()
    print('✓ Configuración enviada correctamente')
    
except Exception as e:
    print(f'Advertencia: Error de configuración: {e}')
    print('Configuración manual requerida.')
    print('Comandos a enviar:')
    for cmd in commands:
        print(f'  {cmd}')
    
" 2>nul

if exist temp_config.txt del temp_config.txt

echo.
echo ===============================================
echo           FLASHEO COMPLETADO
echo ===============================================
echo.
echo Dispositivo configurado como: %ROLE% (ID: %DEVICE_ID%)
echo Red: %CHANNEL% (Password: %PASSWORD%)
echo.
echo Para monitorear el dispositivo:
echo   py -m platformio device monitor --baud 115200
echo.
echo Presiona cualquier tecla para abrir el monitor serial...
pause >nul

REM Abrir monitor serial en nueva ventana
start "Monitor Serial ESP32" cmd /k "py -m platformio device monitor --baud 115200 --port %ESP_PORT%"

echo.
echo Monitor serial abierto en nueva ventana.
echo ¡Listo para operar!
echo.
pause