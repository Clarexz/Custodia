@echo off
setlocal enabledelayedexpansion
title Custodia Flash Tool
color 0A

echo.
echo ========================================
echo        CUSTODIA FLASH TOOL v2.0        
echo           Windows Native Version       
echo ========================================
echo.
echo This tool will help you configure and flash your ESP32 device.
echo.
echo Current directory: %CD%
echo.

REM Check if we're in the right directory
if not exist "platformio.ini" (
    echo [ERROR] platformio.ini not found
    echo Please run this tool from the Custodia project root directory
    echo.
    echo Current directory contents:
    dir /b
    echo.
    pause
    exit /b 1
)

echo [OK] Found platformio.ini - project structure OK
echo.

REM Check for Python (py or python)
echo [INFO] Checking Python installation...
set "PYCMD="
where py >nul 2>&1 && set "PYCMD=py"
if not defined PYCMD (
    where python >nul 2>&1 && set "PYCMD=python"
)
if not defined PYCMD (
    echo [ERROR] Python not found
    echo Install Python from https://python.org or Microsoft Store
    echo Ensure it is available as 'py' or 'python' in PATH
    echo.
    pause
    exit /b 1
)

echo [OK] Python found:
%PYCMD% --version

REM Check for PlatformIO
echo [INFO] Checking PlatformIO installation...
REM Prefer module invocation to avoid PATH issues
set "PIO_CMD=%PYCMD% -m platformio"
%PIO_CMD% --version >nul 2>&1
if %errorlevel% neq 0 (
    REM Try pio from PATH
    where pio >nul 2>&1 && set "PIO_CMD=pio"
    %PIO_CMD% --version >nul 2>&1
)
if %errorlevel% neq 0 (
    echo [WARN] PlatformIO not found or not responding. Attempting install/repair...
    %PYCMD% -m pip install --upgrade pip
    %PYCMD% -m pip install -U platformio
    REM Try again (module first)
    set "PIO_CMD=%PYCMD% -m platformio"
    %PIO_CMD% --version >nul 2>&1
    if %errorlevel% neq 0 (
        REM As a last resort, don't abort; warn and continue. Many environments still allow run commands even if --version fails (Python 3.13 quirks)
        echo [WARN] PlatformIO version check failed. I will continue using "%PYCMD% -m platformio".
        set "PIO_CMD=%PYCMD% -m platformio"
    ) else (
        echo [OK] PlatformIO installed and available (module)
    )
) else (
    echo [OK] PlatformIO available
)

REM Ensure pyserial is installed
echo [INFO] Checking pyserial...
%PYCMD% -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARN] pyserial not found. Installing...
    %PYCMD% -m pip install pyserial
)

echo.
echo ========================================
echo          CONFIGURATION MODE            
echo ========================================
echo.

REM If invoked with any parameters or the special --one flag, skip mode selection (one-line mode)
if /i "%~1"=="--one" goto parse_one_line_args
if not "%~1"=="" goto parse_one_line_args

echo Select configuration type:
echo   1. Normal     - Interactive step-by-step configuration
echo   2. One Line   - Single command line configuration
echo.
set /p "CFG_CHOICE=Select [1-2]: "
if "%CFG_CHOICE%"=="2" (
    echo.
    echo Example:
    echo -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -hops 3 -channel TEAM_ALPHA -password Secure123
    echo.
    set /p "CONFIG_PARAMS=Enter your configuration parameters: "
    REM Re-invoke with a sentinel to guarantee argument detection in a fresh cmd
    %ComSpec% /c ""%~f0" --one %CONFIG_PARAMS%"
    exit /b %errorlevel%
)

:skip_mode_selection
echo ========================================
echo           CONFIGURATION SETUP          
echo ========================================
echo.

REM If we were called with one-line parameters, parse them and skip interactive prompts
REM (Handled above before menu)

REM Simple interactive configuration
:get_role
echo Device Role:
echo   1. TRACKER  - GPS tracking device
echo   2. RECEIVER - Base station/receiver  
echo   3. REPEATER - Signal repeater/relay
echo.
set /p "role_choice=Select role [1-3]: "

if "%role_choice%"=="1" (
    set "ROLE=TRACKER"
    goto get_id
) else if "%role_choice%"=="2" (
    set "ROLE=RECEIVER"
    goto get_id
) else if "%role_choice%"=="3" (
    set "ROLE=REPEATER"
    goto get_id
) else (
    echo [ERROR] Invalid selection. Please enter 1, 2, or 3
    goto get_role
)

:get_id
echo.
echo Device ID: Unique identifier for this device (1-255)
set /p "DEVICE_ID=Enter device ID [1-255]: "

if %DEVICE_ID% lss 1 (
    echo [ERROR] Device ID must be between 1 and 255
    goto get_id
)
if %DEVICE_ID% gtr 255 (
    echo [ERROR] Device ID must be between 1 and 255
    goto get_id
)

echo.
echo Transmission Interval: How often device sends GPS updates (in seconds)
echo Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

if %INTERVAL% lss 5 (
    echo [ERROR] Interval must be between 5 and 3600 seconds
    goto get_id
)
if %INTERVAL% gtr 3600 (
    echo [ERROR] Interval must be between 5 and 3600 seconds
    goto get_id
)

echo.
echo LoRa Region:
echo   1. US - United States
echo   2. EU - Europe
echo   3. CH - China
echo   4. AS - Asia
echo   5. JP - Japan
echo.
set /p "region_choice=Select region [1-5]: "

if "%region_choice%"=="1" (
    set "REGION=US"
    goto get_mode
) else if "%region_choice%"=="2" (
    set "REGION=EU"
    goto get_mode
) else if "%region_choice%"=="3" (
    set "REGION=CH"
    goto get_mode
) else if "%region_choice%"=="4" (
    set "REGION=AS"
    goto get_mode
) else if "%region_choice%"=="5" (
    set "REGION=JP"
    goto get_mode
) else (
    echo [ERROR] Invalid selection. Please enter 1, 2, 3, 4, or 5
    goto get_id
)

:get_mode
echo.
echo Operation Mode:
echo   1. SIMPLE - Basic operation
echo   2. ADMIN  - Advanced features enabled
echo.
set /p "mode_choice=Select mode [1-2]: "

if "%mode_choice%"=="1" (
    set "MODE=SIMPLE"
    goto get_radio
) else if "%mode_choice%"=="2" (
    set "MODE=ADMIN"
    goto get_radio
) else (
    echo [ERROR] Invalid selection. Please enter 1 or 2
    goto get_mode
)

:get_radio
echo.
echo Radio Profile:
echo   1. DESERT_LONG_FAST  - Long range, fast transmission
echo   2. MOUNTAIN_STABLE   - Stable connection in mountains
echo   3. URBAN_DENSE       - Dense urban environments
echo   4. MESH_MAX_NODES    - Maximum mesh network nodes
echo   5. CUSTOM_ADVANCED   - Custom advanced settings
echo.
set /p "radio_choice=Select radio profile [1-5]: "
if "%radio_choice%"=="1" set "RADIO=DESERT_LONG_FAST" & goto get_hops
if "%radio_choice%"=="2" set "RADIO=MOUNTAIN_STABLE" & goto get_hops
if "%radio_choice%"=="3" set "RADIO=URBAN_DENSE" & goto get_hops
if "%radio_choice%"=="4" set "RADIO=MESH_MAX_NODES" & goto get_hops
if "%radio_choice%"=="5" set "RADIO=CUSTOM_ADVANCED" & goto custom_adv
echo [ERROR] Invalid selection. Please enter 1-5
goto get_radio

:custom_adv
echo.
echo Custom Advanced Radio Parameters:
echo.

:adv_bw
echo Bandwidth (kHz):
echo   1. 125  2. 250  3. 500
set /p "advb=Select [1-3]: "
if "%advb%"=="1" (set "CUSTOM_BW=125") else if "%advb%"=="2" (set "CUSTOM_BW=250") else if "%advb%"=="3" (set "CUSTOM_BW=500") else (echo Invalid & goto adv_bw)

:adv_sf
echo Spreading Factor:
echo   1.SF7 2.SF8 3.SF9 4.SF10 5.SF11 6.SF12
set /p "advs=Select [1-6]: "
if "%advs%"=="1" (set "CUSTOM_SF=7") else if "%advs%"=="2" (set "CUSTOM_SF=8") else if "%advs%"=="3" (set "CUSTOM_SF=9") else if "%advs%"=="4" (set "CUSTOM_SF=10") else if "%advs%"=="5" (set "CUSTOM_SF=11") else if "%advs%"=="6" (set "CUSTOM_SF=12") else (echo Invalid & goto adv_sf)

:adv_cr
echo Coding Rate:
echo   1.4/5 2.4/6 3.4/7 4.4/8
set /p "advc=Select [1-4]: "
if "%advc%"=="1" (set "CUSTOM_CR=5") else if "%advc%"=="2" (set "CUSTOM_CR=6") else if "%advc%"=="3" (set "CUSTOM_CR=7") else if "%advc%"=="4" (set "CUSTOM_CR=8") else (echo Invalid & goto adv_cr)

:adv_pw
set /p "CUSTOM_POWER=TX Power (2-22 dBm): "
if "%CUSTOM_POWER%"=="" (echo Invalid & goto adv_pw)
for /f "delims=0123456789" %%x in ("%CUSTOM_POWER%") do (echo Invalid & goto adv_pw)
if %CUSTOM_POWER% lss 2 (echo Invalid & goto adv_pw)
if %CUSTOM_POWER% gtr 22 (echo Invalid & goto adv_pw)

:adv_pr
set /p "CUSTOM_PREAMBLE=Preamble Length (6-65535): "
if "%CUSTOM_PREAMBLE%"=="" (echo Invalid & goto adv_pr)
for /f "delims=0123456789" %%x in ("%CUSTOM_PREAMBLE%") do (echo Invalid & goto adv_pr)
if %CUSTOM_PREAMBLE% lss 6 (echo Invalid & goto adv_pr)
if %CUSTOM_PREAMBLE% gtr 65535 (echo Invalid & goto adv_pr)
goto get_hops

:get_hops
echo.
echo Max Hops (1-10) [Default 3]:
set /p "HOPS=Enter hops [1-10] or press Enter for 3: "
if "%HOPS%"=="" set "HOPS=3"
if %HOPS% lss 1 (echo Invalid; goto get_hops)
if %HOPS% gtr 10 (echo Invalid; goto get_hops)

:get_channel
echo.
echo Network Channel: Network name for your devices (3-20 characters)
echo Only letters, numbers, and underscore allowed
echo Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS
set /p "CHANNEL=Enter channel name: "

if "%CHANNEL%"=="" (
    echo [ERROR] Channel name cannot be empty
    goto get_channel
)

echo.
echo Network Password: Password for your network (8-32 characters)
echo Must contain at least 1 number and 1 letter
echo Examples: Secure123, MyPass99, Field2024
set /p "PASSWORD=Enter password (or press Enter for auto-generation): "

if "%PASSWORD%"=="" (
    set "PASSWORD=Secure123"
    echo [INFO] Password auto-generated: %PASSWORD%
)

echo.
echo ========================================
echo           CONFIGURATION SUMMARY        
echo ========================================
echo Role: %ROLE%
echo ID: %DEVICE_ID%
echo Interval: %INTERVAL% seconds
echo Region: %REGION%
echo Mode: %MODE%
echo Radio: %RADIO%
echo Hops: %HOPS%
echo Channel: %CHANNEL%
echo Password: %PASSWORD%
echo ========================================
echo.

set /p "confirm=Proceed with flashing? (Y/n): "
if /i "%confirm%"=="n" (
    echo [INFO] Operation cancelled by user
    pause
    exit /b 0
)

echo.
goto device_detection

:parse_one_line_args
REM Initialize variables
REM If called with sentinel, drop it
if /i "%~1"=="--one" shift
set "ROLE="
set "DEVICE_ID="
set "INTERVAL="
set "REGION="
set "MODE="
set "RADIO="
set "HOPS="
set "CHANNEL="
set "PASSWORD="
set "PORT_OVERRIDE="

:_arg_loop
if "%~1"=="" goto _args_done
if /i "%~1"=="-role" (
    set "ROLE=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-id" (
    set "DEVICE_ID=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-interval" (
    set "INTERVAL=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-region" (
    set "REGION=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-mode" (
    set "MODE=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-radio" (
    set "RADIO=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-hops" (
    set "HOPS=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-channel" (
    set "CHANNEL=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-password" (
    set "PASSWORD=%~2"
    shift & shift & goto _arg_loop
)
if /i "%~1"=="-port" (
    set "PORT_OVERRIDE=%~2"
    shift & shift & goto _arg_loop
)
REM Unrecognized token, skip it
shift
goto _arg_loop

:_args_done
if not defined HOPS set "HOPS=3"

echo.
echo ========================================
echo           CONFIGURATION SUMMARY        
echo ========================================
echo Role: %ROLE%
echo ID: %DEVICE_ID%
echo Interval: %INTERVAL% seconds
echo Region: %REGION%
echo Mode: %MODE%
echo Radio: %RADIO%
echo Hops: %HOPS%
echo Channel: %CHANNEL%
echo Password: %PASSWORD%
echo ========================================
echo.

REM If user provided a port override, use it later
if defined PORT_OVERRIDE set "SELECTED_PORT=%PORT_OVERRIDE%"

REM Proceed directly to device detection and flashing
goto device_detection

REM ========================
REM Device detection & flash
REM ========================
:device_detection
echo ========================================
echo            DEVICE DETECTION            
echo ========================================
echo.

echo [INFO] Detecting ESP32 devices...
REM If a port was provided/selected earlier, skip auto-detection
if defined SELECTED_PORT goto port_found
for /f "tokens=*" %%i in ('%PYCMD% -c "import serial.tools.list_ports; ports = serial.tools.list_ports.comports(); [print(port.device) for port in ports if 'com' in port.device.lower()]" 2^>nul') do (
    set "_PORTS_LIST=!_PORTS_LIST! %%i"
    set /a _PORTS_COUNT+=1
)
if not defined _PORTS_COUNT set "_PORTS_COUNT=0"
if %_PORTS_COUNT% EQU 0 (
    echo [ERROR] No COM ports found
    pause
    exit /b 1
) else if %_PORTS_COUNT% EQU 1 (
    for %%p in (%_PORTS_LIST%) do set "SELECTED_PORT=%%p"
    echo [OK] Auto-selected port: %SELECTED_PORT%
    goto port_found
) else (
    echo Available ports:
    set _idx=0
    for %%p in (%_PORTS_LIST%) do (
        set /a _idx+=1
        echo   !_idx!. %%p
    )
)

:choose_port
echo.
set /p "PORT_CHOICE=Select port number (or press Enter to re-display the list): "

if "%PORT_CHOICE%"=="" (
    echo [INFO] Re-displaying available ports...
    echo Available ports:
    set _idx=0
    for %%p in (%_PORTS_LIST%) do (
        set /a _idx+=1
        echo   !_idx!. %%p
    )
    goto choose_port
) else (
    REM Get port by number
    set "PORT_INDEX=0"
    for %%i in (%_PORTS_LIST%) do (
        set /a PORT_INDEX+=1
        if !PORT_INDEX!==%PORT_CHOICE% (
            set "SELECTED_PORT=%%i"
            goto port_found
        )
    )
    echo [ERROR] Invalid port selection. Please enter a valid number from the list.
    goto choose_port
)

:port_found
echo [OK] Selected port: %SELECTED_PORT%

echo.
echo ========================================
echo             FIRMWARE FLASH             
echo ========================================
echo.

echo [INFO] Flashing firmware to %SELECTED_PORT%...
%PIO_CMD% run -e seeed_xiao_esp32s3 --target upload --upload-port %SELECTED_PORT%
if %errorlevel% neq 0 (
    echo [ERROR] Firmware flash failed
    echo.
    echo Manual solution:
    echo 1. Close any open serial monitors
    echo 2. Hold BOOT button on ESP32
    echo 3. Press and release RESET button  
    echo 4. Release BOOT button
    echo 5. Try again
    echo.
    pause
    exit /b 1
)

echo [OK] Firmware flashed successfully!
echo.
echo [INFO] Waiting for device to reboot...
timeout /t 8 /nobreak >nul

echo.
echo ========================================
echo           DEVICE CONFIGURATION         
echo ========================================
echo.

echo [INFO] Configuring device parameters...

REM Create Python script for configuration
set "CONFIG_SCRIPT=%TEMP%\configure_device.py"
(
echo import serial
echo import time
echo.
echo try:
echo     ser = serial.Serial^('%SELECTED_PORT%', 115200, timeout=15^)
echo     time.sleep^(5^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'STATUS\r\n'^)
echo     time.sleep^(2^)
echo     while ser.in_waiting ^> 0:
echo         ser.read^(ser.in_waiting^)
echo         time.sleep^(0.1^)
echo     print^('Device communication established'^)
echo     ser.write^(^('NETWORK_CREATE %CHANNEL% %PASSWORD%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     ser.write^(^('Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     ser.write^(b'CONFIG_SAVE\r\n'^)
echo     time.sleep^(3^)
echo     ser.write^(b'START\r\n'^)
echo     time.sleep^(2^)
echo     ser.close^(^)
echo     print^('Configuration completed successfully'^)
echo except Exception as e:
echo     print^('Configuration error:', str^(e^)^)
) > "%CONFIG_SCRIPT%"

%PYCMD% "%CONFIG_SCRIPT%"
del "%CONFIG_SCRIPT%" 2>nul

echo.
echo [SUCCESS] STATUS: READY FOR OPERATION
echo.
echo Launching serial monitor in a new window...
start "Custodia Monitor" cmd /k "%PIO_CMD% device monitor --baud 115200 --port %SELECTED_PORT%"
echo.
echo If the window didn't open, run:
echo %PIO_CMD% device monitor --baud 115200 --port %SELECTED_PORT%
echo.
pause
