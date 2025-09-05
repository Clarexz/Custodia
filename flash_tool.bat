@echo off
setlocal enabledelayedexpansion

echo ===============================================
echo    Flash Tool ESP32-S3 (Completamente Automático)
echo ===============================================

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

REM Procesar argumentos
:parse_args
if "%~1"=="" goto check_params
if "%~1"=="-role" (set "ROLE=%~2" & shift & shift & goto parse_args)
if "%~1"=="-id" (set "DEVICE_ID=%~2" & shift & shift & goto parse_args)
if "%~1"=="-interval" (set "INTERVAL=%~2" & shift & shift & goto parse_args)
if "%~1"=="-region" (set "REGION=%~2" & shift & shift & goto parse_args)
if "%~1"=="-mode" (set "MODE=%~2" & shift & shift & goto parse_args)
if "%~1"=="-radio" (set "RADIO=%~2" & shift & shift & goto parse_args)
if "%~1"=="-hops" (set "HOPS=%~2" & shift & shift & goto parse_args)
if "%~1"=="-channel" (set "CHANNEL=%~2" & shift & shift & goto parse_args)
if "%~1"=="-password" (set "PASSWORD=%~2" & shift & shift & goto parse_args)
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help
shift & goto parse_args

:show_help
echo Uso: flash_auto.bat [opciones]
echo   -role ROLE        TRACKER/REPEATER/RECEIVER
echo   -id ID            1-255
echo   -interval SEC     5-3600
echo   -region REGION    US/EU/CH/AS/JP
echo   -channel NAME     Nombre de red
echo   -password PASS    Contraseña (opcional)
exit /b 0

:check_params
REM Generar password si no se proporcionó
if "%PASSWORD%"=="" set "PASSWORD=AUTO%random:~-4%"

echo Configuración:
echo   Rol: %ROLE%
echo   ID: %DEVICE_ID%
echo   Canal: %CHANNEL%
echo   Password: %PASSWORD%
echo.

REM 1. Verificar Python
echo [1/5] Verificando Python...
py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python no encontrado
    pause & exit /b 1
)
echo ✓ Python OK

REM 2. Instalar dependencias
echo.
echo [2/5] Instalando dependencias...
py -m pip install platformio >nul 2>&1
echo ✓ PlatformIO listo

REM 3. Detectar puerto ESP32
echo.
echo [3/5] Detectando ESP32...

REM Crear script PowerShell temporal para detectar puerto
echo $ports = [System.IO.Ports.SerialPort]::getportnames() > detect_port.ps1
echo foreach ($port in $ports) { >> detect_port.ps1
echo   try { >> detect_port.ps1
echo     $serial = New-Object System.IO.Ports.SerialPort($port, 115200) >> detect_port.ps1
echo     $serial.Open() >> detect_port.ps1
echo     $serial.Close() >> detect_port.ps1
echo     Write-Host $port >> detect_port.ps1
echo     break >> detect_port.ps1
echo   } catch { } >> detect_port.ps1
echo } >> detect_port.ps1

REM Ejecutar detección de puerto
for /f %%i in ('powershell -ExecutionPolicy Bypass -File detect_port.ps1 2^>nul') do set "ESP_PORT=%%i"

if "%ESP_PORT%"=="" (
    echo ERROR: No se detectó ESP32
    echo Conecta el ESP32-S3 via USB y reintenta
    del detect_port.ps1 2>nul
    pause & exit /b 1
)

echo ✓ ESP32 detectado en: %ESP_PORT%
del detect_port.ps1 2>nul

REM 4. Compilar y flashear
echo.
echo [4/5] Compilando y flasheando...
py -m platformio run
if %errorlevel% neq 0 (
    echo ERROR: Falló compilación
    pause & exit /b 1
)

py -m platformio run --target upload
if %errorlevel% neq 0 (
    echo ERROR: Falló flasheo - Presiona BOOT+RESET en ESP32 y reintenta
    pause & exit /b 1
)
echo ✓ Firmware flasheado

REM 5. Configuración automática via PowerShell
echo.
echo [5/5] Configurando dispositivo...
echo Esperando reinicio del dispositivo...
ping localhost -n 8 >nul

REM Crear script PowerShell para configuración serial
echo $port = New-Object System.IO.Ports.SerialPort('%ESP_PORT%', 115200) > config_device.ps1
echo $port.Open() >> config_device.ps1
echo Start-Sleep -Seconds 2 >> config_device.ps1
echo $commands = @( >> config_device.ps1
echo   'CONFIG_NETWORK %CHANNEL% %PASSWORD%', >> config_device.ps1
echo   'CONFIG_BASIC %ROLE% %DEVICE_ID% %INTERVAL% %REGION% %MODE% %RADIO% %HOPS%', >> config_device.ps1
echo   'CONFIG_SAVE', >> config_device.ps1
echo   'START' >> config_device.ps1
echo ^) >> config_device.ps1
echo foreach ($cmd in $commands^) { >> config_device.ps1
echo   Write-Host "Enviando: $cmd" >> config_device.ps1
echo   $port.WriteLine($cmd^) >> config_device.ps1
echo   Start-Sleep -Seconds 1 >> config_device.ps1
echo } >> config_device.ps1
echo $port.Close() >> config_device.ps1
echo Write-Host "Configuración completada" >> config_device.ps1

REM Ejecutar configuración
powershell -ExecutionPolicy Bypass -File config_device.ps1 2>nul
if %errorlevel% neq 0 (
    echo Advertencia: Error en configuración automática
    echo Configuración manual requerida:
    echo   CONFIG_NETWORK %CHANNEL% %PASSWORD%
    echo   CONFIG_BASIC %ROLE% %DEVICE_ID% %INTERVAL% %REGION% %MODE% %RADIO% %HOPS%
    echo   CONFIG_SAVE
    echo   START
) else (
    echo ✓ Configuración automática completada
)

del config_device.ps1 2>nul

echo.
echo ===============================================
echo           PROCESO COMPLETADO
echo ===============================================
echo.
echo Dispositivo: %ROLE% (ID: %DEVICE_ID%)
echo Red: %CHANNEL% / %PASSWORD%
echo Puerto: %ESP_PORT%
echo.
echo ¿Abrir monitor serial? (S/N)
set /p respuesta=
if /i "%respuesta%"=="S" (
    start "Monitor ESP32" cmd /k "py -m platformio device monitor --baud 115200 --port %ESP_PORT%"
)

echo ¡Dispositivo listo para operar!
pause