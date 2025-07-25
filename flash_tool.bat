@echo off
setlocal EnableDelayedExpansion

REM =====================================================
REM  CUSTOM MESHTASTIC FLASH TOOL v2.0
REM  Auto-flash and configure ESP32-S3 devices
REM  Using named parameters for clarity
REM =====================================================

echo.
echo =========================================================
echo  CUSTOM MESHTASTIC ESP32-S3 FLASH TOOL v2.0
echo =========================================================
echo.

REM Initialize variables
set ROLE=
set DEVICE_ID=
set GPS_INTERVAL=
set REGION=
set DATA_MODE=
set RADIO_PROFILE=
set MAX_HOPS=3

REM Parse named parameters
:parse_args
if "%~1"=="" goto :validate_args

if /i "%1"=="-role" (
    set ROLE=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-id" (
    set DEVICE_ID=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-gps" (
    set GPS_INTERVAL=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-region" (
    set REGION=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-mode" (
    set DATA_MODE=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-radio" (
    set RADIO_PROFILE=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-hops" (
    set MAX_HOPS=%2
    shift
    shift
    goto :parse_args
)

if /i "%1"=="-help" goto :show_usage
if /i "%1"=="--help" goto :show_usage
if /i "%1"=="/?" goto :show_usage

echo ERROR: Parametro desconocido: %1
goto :show_usage

:validate_args
REM Check required parameters
if "%ROLE%"=="" (
    echo ERROR: Parametro -role es obligatorio
    goto :show_usage
)

if "%DEVICE_ID%"=="" (
    echo ERROR: Parametro -id es obligatorio
    goto :show_usage
)

if "%GPS_INTERVAL%"=="" (
    echo ERROR: Parametro -gps es obligatorio
    goto :show_usage
)

if "%REGION%"=="" (
    echo ERROR: Parametro -region es obligatorio
    goto :show_usage
)

if "%DATA_MODE%"=="" (
    echo ERROR: Parametro -mode es obligatorio
    goto :show_usage
)

if "%RADIO_PROFILE%"=="" (
    echo ERROR: Parametro -radio es obligatorio
    goto :show_usage
)

echo Configuracion a aplicar:
echo - Rol: %ROLE%
echo - Device ID: %DEVICE_ID%
echo - GPS Interval: %GPS_INTERVAL%s
echo - Region: %REGION%
echo - Data Mode: %DATA_MODE%
echo - Radio Profile: %RADIO_PROFILE%
echo - Max Hops: %MAX_HOPS%
echo.

REM Parameter validation
set VALID_ROLES=TRACKER REPEATER RECEIVER
echo %VALID_ROLES% | findstr /i /w "%ROLE%" >nul
if errorlevel 1 (
    echo ERROR: Rol invalido: %ROLE%
    echo Roles validos: TRACKER, REPEATER, RECEIVER
    goto :end
)

if %DEVICE_ID% LSS 1 (
    echo ERROR: Device ID debe ser mayor a 0
    goto :end
)
if %DEVICE_ID% GTR 999 (
    echo ERROR: Device ID debe ser menor a 1000
    goto :end
)

if %GPS_INTERVAL% LSS 5 (
    echo ERROR: GPS Interval debe ser mayor o igual a 5 segundos
    goto :end
)
if %GPS_INTERVAL% GTR 3600 (
    echo ERROR: GPS Interval debe ser menor o igual a 3600 segundos
    goto :end
)

set VALID_REGIONS=US EU CH AS JP
echo %VALID_REGIONS% | findstr /i /w "%REGION%" >nul
if errorlevel 1 (
    echo ERROR: Region invalida: %REGION%
    echo Regiones validas: US, EU, CH, AS, JP
    goto :end
)

set VALID_MODES=SIMPLE ADMIN
echo %VALID_MODES% | findstr /i /w "%DATA_MODE%" >nul
if errorlevel 1 (
    echo ERROR: Modo de datos invalido: %DATA_MODE%
    echo Modos validos: SIMPLE, ADMIN
    goto :end
)

set VALID_PROFILES=DESERT_LONG_FAST MOUNTAIN_STABLE URBAN_DENSE MESH_MAX_NODES CUSTOM_ADVANCED
echo %VALID_PROFILES% | findstr /i /w "%RADIO_PROFILE%" >nul
if errorlevel 1 (
    echo ERROR: Radio Profile invalido: %RADIO_PROFILE%
    echo Profiles validos: DESERT_LONG_FAST, MOUNTAIN_STABLE, URBAN_DENSE, MESH_MAX_NODES, CUSTOM_ADVANCED
    goto :end
)

if %MAX_HOPS% LSS 1 (
    echo ERROR: Max Hops debe ser mayor o igual a 1
    goto :end
)
if %MAX_HOPS% GTR 10 (
    echo ERROR: Max Hops debe ser menor o igual a 10
    goto :end
)

echo =========================================================
echo  INICIANDO PROCESO DE FLASH
echo =========================================================
echo.

REM Step 1: Detectar puerto ESP32-S3
echo [1/4] Detectando puerto ESP32-S3...
echo.

REM Crear archivo temporal para capturar salida
set TEMP_FILE=%TEMP%\esp_ports.txt
pio device list > "%TEMP_FILE%" 2>&1

REM Buscar puerto con ESP32-S3
set ESP_PORT=
for /f "tokens=*" %%i in ('findstr /i "ESP32-S3\|ESP32S3\|CP210\|CH340\|CH341" "%TEMP_FILE%"') do (
    for /f "tokens=1" %%j in ("%%i") do (
        set POTENTIAL_PORT=%%j
        if "!POTENTIAL_PORT:~0,3!"=="COM" (
            set ESP_PORT=!POTENTIAL_PORT!
            goto :port_found
        )
    )
)

:port_found
if "%ESP_PORT%"=="" (
    echo ERROR: No se encontro ESP32-S3 conectado
    echo.
    echo Asegurese de que:
    echo - El ESP32-S3 este conectado via USB
    echo - Los drivers esten instalados
    echo - El dispositivo este en modo bootloader si es necesario
    echo.
    pause
    goto :end
)

echo Puerto detectado: %ESP_PORT%
echo.

REM Step 2: Verificar PlatformIO
echo [2/4] Verificando PlatformIO...
where pio >nul 2>nul
if errorlevel 1 (
    echo ERROR: PlatformIO no encontrado en PATH
    echo Instale PlatformIO Core o ejecute desde terminal de PlatformIO
    pause
    goto :end
)
echo PlatformIO encontrado ✓
echo.

REM Step 3: Flash del firmware
echo [3/4] Flasheando firmware...
echo.
echo Compilando y subiendo firmware...
echo Esto puede tomar 1-2 minutos...
echo.

pio run --target upload --upload-port %ESP_PORT%

if errorlevel 1 (
    echo.
    echo ERROR: Fallo el flash del firmware
    echo.
    echo Posibles soluciones:
    echo 1. Mantener presionado BOOT, presionar RESET, soltar BOOT
    echo 2. Verificar conexiones de hardware
    echo 3. Verificar que el puerto sea correcto
    echo.
    pause
    goto :end
)

echo.
echo Firmware flasheado exitosamente ✓
echo.

REM Step 4: Configurar dispositivo
echo [4/4] Configurando dispositivo...
echo.

REM Esperar que el dispositivo se reinicie
echo Esperando reinicio del dispositivo...
timeout /t 5 /nobreak >nul

REM Crear comando de configuracion completo
if "%MAX_HOPS%"=="3" (
    set CONFIG_CMD=Q_CONFIG %ROLE%,%DEVICE_ID%,%GPS_INTERVAL%,%REGION%,%DATA_MODE%,%RADIO_PROFILE%
) else (
    set CONFIG_CMD=Q_CONFIG %ROLE%,%DEVICE_ID%,%GPS_INTERVAL%,%REGION%,%DATA_MODE%,%RADIO_PROFILE%,%MAX_HOPS%
)

echo Enviando configuracion: %CONFIG_CMD%
echo.

REM Usar PowerShell para enviar comando al puerto serial
powershell -Command "$port = New-Object System.IO.Ports.SerialPort '%ESP_PORT%', 115200; $port.Open(); Start-Sleep 2; $port.WriteLine('%CONFIG_CMD%'); Start-Sleep 3; $port.Close()"

if errorlevel 1 (
    echo WARNING: Error enviando configuracion automatica
    echo Puede configurar manualmente con:
    echo   %CONFIG_CMD%
) else (
    echo Configuracion enviada exitosamente ✓
)

echo.
echo =========================================================
echo  ✅ PROCESO COMPLETADO
echo =========================================================
echo.
echo Configuracion aplicada:
echo - Rol: %ROLE%
echo - Device ID: %DEVICE_ID%  
echo - GPS Interval: %GPS_INTERVAL% segundos
echo - Region: %REGION%
echo - Data Mode: %DATA_MODE%
echo - Radio Profile: %RADIO_PROFILE%
echo - Max Hops: %MAX_HOPS%
echo.
echo El dispositivo deberia estar funcionando en modo operativo.
echo.
echo Para monitorear el dispositivo:
echo   pio device monitor --port %ESP_PORT% --baud 115200
echo.
goto :success_end

:show_usage
echo.
echo CUSTOM MESHTASTIC FLASH TOOL v2.0
echo.
echo MODO DE USO:
echo   flash_tool.bat -role ROLE -id ID -gps INTERVAL -region REGION -mode MODE -radio PROFILE [-hops HOPS]
echo.
echo PARAMETROS OBLIGATORIOS:
echo   -role ROLE       : Rol del dispositivo (TRACKER, REPEATER, RECEIVER)
echo   -id ID          : Device ID unico (1-999)  
echo   -gps INTERVAL   : Intervalo GPS en segundos (5-3600)
echo   -region REGION  : Region LoRa (US, EU, CH, AS, JP)
echo   -mode MODE      : Modo de visualizacion (SIMPLE, ADMIN)
echo   -radio PROFILE  : Perfil de radio LoRa (ver abajo)
echo.
echo PARAMETROS OPCIONALES:
echo   -hops HOPS      : Maximo saltos mesh (1-10, default=3)
echo   -help           : Mostrar esta ayuda
echo.
echo EJEMPLOS:
echo   flash_tool.bat -role TRACKER -id 001 -gps 15 -region US -mode SIMPLE -radio MESH_MAX_NODES
echo   flash_tool.bat -role REPEATER -id 002 -gps 30 -region EU -mode ADMIN -radio DESERT_LONG_FAST -hops 5
echo   flash_tool.bat -role RECEIVER -id 003 -gps 60 -region US -mode SIMPLE -radio MOUNTAIN_STABLE
echo.
echo RADIO PROFILES DISPONIBLES:
echo   DESERT_LONG_FAST   - Maximo alcance (~8km, bateria 3/10, velocidad 2/10)
echo                        Ideal para: Animal tracking, field monitoring, sensores remotos
echo.
echo   MOUNTAIN_STABLE    - Condiciones adversas (~4km, bateria 5/10, velocidad 4/10)
echo                        Ideal para: Repetidores en bosques, despliegues montañosos
echo.
echo   URBAN_DENSE        - Alta velocidad (~800m, bateria 8/10, velocidad 9/10)
echo                        Ideal para: Testing en laboratorio, IoT urbano, desarrollo
echo.
echo   MESH_MAX_NODES     - Balance para redes grandes (~2.5km, bateria 7/10, velocidad 7/10)
echo                        Ideal para: Redes mesh de 20-30 nodos, balance general
echo.
echo   CUSTOM_ADVANCED    - Configuracion manual experta
echo                        Ideal para: Requisitos especificos, configuracion avanzada
echo.
echo REGIONES LoRa:
echo   US  - Estados Unidos, Mexico, Canada (915 MHz)
echo   EU  - Europa, Africa, Medio Oriente (868 MHz)
echo   CH  - China (470 MHz)
echo   AS  - Asia general (433 MHz)
echo   JP  - Japon (920 MHz)
echo.

:success_end
echo Presione cualquier tecla para salir...
pause >nul

:end
endlocal