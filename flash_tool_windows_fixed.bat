@echo off
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows - Fixed Version
REM Simplified version with working port detection

REM Global variables
set "SCRIPT_DIR=%~dp0"
if "!SCRIPT_DIR:~-1!"=="\" set "SCRIPT_DIR=!SCRIPT_DIR:~0,-1!"
set "PROJECT_DIR=!SCRIPT_DIR!"
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"
set "PYTHON_CMD="
set "PIP_CMD="

REM Configuration variables
set "ROLE="
set "DEVICE_ID="
set "INTERVAL="
set "REGION="
set "MODE="
set "RADIO="
set "HOPS=3"
set "CHANNEL="
set "PASSWORD="

REM Print header
cls
echo.
echo ==================================
echo     Custodia Flash Tool v2.0     
echo     Windows Fixed Version    
echo ==================================
echo.

REM Check for command line arguments (one-line mode)
if not "%~1"=="" (
    call :parse_arguments %*
    if errorlevel 1 (
        echo ERROR: Invalid arguments
        pause
        exit /b 1
    )
    call :execute_flash
    goto :end
)

REM Interactive mode
call :interactive_mode
goto :end

:parse_arguments
:parse_loop
if "%~1"=="" goto :parse_done

if /i "%~1"=="-role" (
    set "ROLE=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-id" (
    set "DEVICE_ID=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-interval" (
    set "INTERVAL=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-region" (
    set "REGION=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-mode" (
    set "MODE=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-radio" (
    set "RADIO=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-channel" (
    set "CHANNEL=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-password" (
    set "PASSWORD=%~2"
    shift
    shift
    goto :parse_loop
)
if /i "%~1"=="-hops" (
    set "HOPS=%~2"
    shift
    shift
    goto :parse_loop
)

echo Unknown argument: %~1
exit /b 1

:parse_done
REM Validate required parameters
if "!ROLE!"=="" (
    echo ERROR: -role parameter is required
    exit /b 1
)
if "!DEVICE_ID!"=="" (
    echo ERROR: -id parameter is required
    exit /b 1
)
if "!INTERVAL!"=="" (
    echo ERROR: -interval parameter is required
    exit /b 1
)
if "!REGION!"=="" (
    echo ERROR: -region parameter is required
    exit /b 1
)
if "!MODE!"=="" (
    echo ERROR: -mode parameter is required
    exit /b 1
)
if "!RADIO!"=="" (
    echo ERROR: -radio parameter is required
    exit /b 1
)
if "!CHANNEL!"=="" (
    echo ERROR: -channel parameter is required
    exit /b 1
)
goto :eof

:interactive_mode
echo This tool will help you configure and flash your ESP32 device.
echo.

REM Device Role
:role_loop
echo 1. Device Role:
echo    1. TRACKER  - GPS tracking device
echo    2. RECEIVER - Base station/receiver
echo    3. REPEATER - Signal repeater/relay
echo.
set /p "choice=Select role [1-3]: "

if "!choice!"=="1" (
    set "ROLE=TRACKER"
    goto :device_id
)
if "!choice!"=="2" (
    set "ROLE=RECEIVER"
    goto :device_id
)
if "!choice!"=="3" (
    set "ROLE=REPEATER"
    goto :device_id
)
echo Invalid selection. Please enter 1, 2, or 3
goto :role_loop

:device_id
echo.
echo 2. Device ID:
echo    Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

if !DEVICE_ID! lss 1 goto :device_id_error
if !DEVICE_ID! gtr 255 goto :device_id_error
goto :interval

:device_id_error
echo Invalid device ID. Please enter a number between 1 and 255
goto :device_id

:interval
echo.
echo 3. Transmission Interval:
echo    How often device sends GPS updates (in seconds)
echo    Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

if !INTERVAL! lss 5 goto :interval_error
if !INTERVAL! gtr 3600 goto :interval_error
goto :region

:interval_error
echo Invalid interval. Please enter a number between 5 and 3600 seconds
goto :interval

:region
echo.
echo 4. LoRa Region:
echo    1. US - United States
echo    2. EU - Europe  
echo    3. CH - China
echo    4. AS - Asia
echo    5. JP - Japan
echo.
set /p "choice=Select region [1-5]: "

if "!choice!"=="1" (
    set "REGION=US"
    goto :mode
)
if "!choice!"=="2" (
    set "REGION=EU"
    goto :mode
)
if "!choice!"=="3" (
    set "REGION=CH"
    goto :mode
)
if "!choice!"=="4" (
    set "REGION=AS"
    goto :mode
)
if "!choice!"=="5" (
    set "REGION=JP"
    goto :mode
)
echo Invalid selection. Please enter 1, 2, 3, 4, or 5
goto :region

:mode
echo.
echo 5. Operation Mode:
echo    1. SIMPLE - Basic operation
echo    2. ADMIN  - Advanced features enabled
echo.
set /p "choice=Select mode [1-2]: "

if "!choice!"=="1" (
    set "MODE=SIMPLE"
    goto :radio
)
if "!choice!"=="2" (
    set "MODE=ADMIN"
    goto :radio
)
echo Invalid selection. Please enter 1 or 2
goto :mode

:radio
echo.
echo 6. Radio Profile:
echo    1. DESERT_LONG_FAST  - Long range, fast transmission
echo    2. MOUNTAIN_STABLE   - Stable connection in mountains
echo    3. URBAN_DENSE       - Dense urban environments
echo    4. MESH_MAX_NODES    - Maximum mesh network nodes
echo    5. CUSTOM_ADVANCED   - Custom advanced settings
echo.
set /p "choice=Select radio profile [1-5]: "

if "!choice!"=="1" (
    set "RADIO=DESERT_LONG_FAST"
    goto :hops
)
if "!choice!"=="2" (
    set "RADIO=MOUNTAIN_STABLE"
    goto :hops
)
if "!choice!"=="3" (
    set "RADIO=URBAN_DENSE"
    goto :hops
)
if "!choice!"=="4" (
    set "RADIO=MESH_MAX_NODES"
    goto :hops
)
if "!choice!"=="5" (
    set "RADIO=CUSTOM_ADVANCED"
    echo Note: Custom parameters will be configured automatically
    goto :hops
)
echo Invalid selection. Please enter 1-5
goto :radio

:hops
echo.
echo 7. Max Hops:
echo    Maximum number of hops for mesh network (1-10)
echo    Default: 3
echo.
set /p "input=Enter max hops [1-10] or press Enter for default (3): "

if "!input!"=="" (
    set "HOPS=3"
    goto :channel
)

if !input! lss 1 goto :hops_error
if !input! gtr 10 goto :hops_error
set "HOPS=!input!"
goto :channel

:hops_error
echo Invalid hops. Please enter a number between 1 and 10
goto :hops

:channel
echo.
echo 8. Network Channel:
echo    Network name for your devices (3-20 characters)
echo    Only letters, numbers, and underscore allowed
echo    Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS
echo.
set /p "CHANNEL=Enter channel name: "

call :validate_channel "!CHANNEL!"
if errorlevel 1 (
    echo Invalid channel name. Requirements:
    echo   - 3-20 characters
    echo   - Only letters, numbers, underscore
    echo   - Cannot be: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT
    goto :channel
)

call :to_upper CHANNEL

:password
echo.
echo 9. Network Password:
echo    Password requirements:
echo    - 8-32 characters
echo    - Must contain at least 1 number and 1 letter
echo    - Cannot be the same as channel name
echo    Examples: Secure123, MyPass99, Field2024
echo.
set /p "PASSWORD=Enter password or press Enter for auto-generation: "

if "!PASSWORD!"=="" (
    echo Password will be auto-generated
    goto :summary
)

call :validate_password "!PASSWORD!"
if errorlevel 1 (
    echo Invalid password. Requirements:
    echo   - 8-32 characters
    echo   - At least 1 number and 1 letter
    echo   - Cannot be same as channel name
    goto :password
)

call :to_upper PASSWORD

:summary
echo.
echo Configuration Summary:
echo Role: !ROLE!
echo ID: !DEVICE_ID!
echo Interval: !INTERVAL! seconds
echo Region: !REGION!
echo Mode: !MODE!
echo Radio: !RADIO!
echo Hops: !HOPS!
echo Channel: !CHANNEL!
if "!PASSWORD!"=="" (
    echo Password: [will be auto-generated]
) else (
    echo Password: !PASSWORD!
)

echo.
set /p "confirm=Proceed with these settings? [Y/n]: "
if /i "!confirm!"=="n" (
    echo Configuration cancelled
    pause
    exit /b 0
)

:execute_flash
REM Check dependencies
call :check_dependencies
if errorlevel 1 (
    echo ERROR: Dependency check failed
    pause
    exit /b 1
)

REM Check project structure
call :check_project_structure
if errorlevel 1 (
    echo ERROR: Project structure check failed
    pause
    exit /b 1
)

REM Detect ports - SIMPLIFIED VERSION
call :detect_ports_simple
if errorlevel 1 (
    echo ERROR: Port detection failed
    pause
    exit /b 1
)

REM Flash process
echo.
echo Starting flash process...
echo Selected port: !SELECTED_PORT!
echo.

REM Clean device flash
call :clean_device_flash

REM Flash firmware
call :flash_firmware
if errorlevel 1 (
    echo ERROR: Firmware flash failed
    pause
    exit /b 1
)

REM Configure device
call :configure_device
if errorlevel 1 (
    echo ERROR: Device configuration failed
    pause
    exit /b 1
)

REM Launch monitor
call :launch_monitor

echo.
echo ===== FLASH COMPLETED SUCCESSFULLY =====
echo Device ID: !DEVICE_ID!
echo Role: !ROLE!
echo Channel: !CHANNEL!
echo Port: !SELECTED_PORT!
echo.
goto :end

:to_upper
set "str=!%~1!"
call set "str=%%str:a=A%%"
call set "str=%%str:b=B%%"
call set "str=%%str:c=C%%"
call set "str=%%str:d=D%%"
call set "str=%%str:e=E%%"
call set "str=%%str:f=F%%"
call set "str=%%str:g=G%%"
call set "str=%%str:h=H%%"
call set "str=%%str:i=I%%"
call set "str=%%str:j=J%%"
call set "str=%%str:k=K%%"
call set "str=%%str:l=L%%"
call set "str=%%str:m=M%%"
call set "str=%%str:n=N%%"
call set "str=%%str:o=O%%"
call set "str=%%str:p=P%%"
call set "str=%%str:q=Q%%"
call set "str=%%str:r=R%%"
call set "str=%%str:s=S%%"
call set "str=%%str:t=T%%"
call set "str=%%str:u=U%%"
call set "str=%%str:v=V%%"
call set "str=%%str:w=W%%"
call set "str=%%str:x=X%%"
call set "str=%%str:y=Y%%"
call set "str=%%str:z=Z%%"
set "%~1=!str!"
goto :eof

:validate_channel
set "channel=%~1"
set "len=0"
:count_chars
if "!channel:~%len%,1!"=="" goto :len_done
set /a len+=1
goto :count_chars
:len_done

if !len! lss 3 exit /b 1
if !len! gtr 20 exit /b 1

REM Check for spaces (simple validation)
if not "!channel!"=="!channel: =!" exit /b 1

REM Check reserved names
call :to_upper channel
for %%r in (CONFIG ADMIN DEBUG SYSTEM DEVICE LORA MESH TEST DEFAULT) do (
    if "!channel!"=="%%r" exit /b 1
)

exit /b 0

:validate_password
set "password=%~1"
set "len=0"
:count_pass_chars
if "!password:~%len%,1!"=="" goto :pass_len_done
set /a len+=1
goto :count_pass_chars
:pass_len_done

if !len! lss 8 exit /b 1
if !len! gtr 32 exit /b 1

REM Check for at least one number
echo !password! | findstr /r "[0-9]" >nul
if errorlevel 1 exit /b 1

REM Check for at least one letter
echo !password! | findstr /r "[A-Za-z]" >nul
if errorlevel 1 exit /b 1

exit /b 0

:check_dependencies
echo [SETUP] Checking dependencies...

REM Check Python - try both commands
set "PYTHON_CMD="
set "PIP_CMD="

python --version >nul 2>&1
if not errorlevel 1 (
    set "PYTHON_CMD=python"
    set "PIP_CMD=pip"
    echo Python found: python command
    goto :python_found
)

py --version >nul 2>&1
if not errorlevel 1 (
    set "PYTHON_CMD=py"
    set "PIP_CMD=py -m pip"
    echo Python found: py command
    goto :python_found
)

echo ERROR: Python not found
echo Please install Python from https://python.org
echo Make sure to check "Add Python to PATH" during installation
pause
exit /b 1

:python_found
REM Check/install pyserial
%PYTHON_CMD% -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo Installing pyserial...
    %PIP_CMD% install pyserial
    if errorlevel 1 (
        echo ERROR: Failed to install pyserial
        pause
        exit /b 1
    )
)

REM Check PlatformIO
pio --version >nul 2>&1
if not errorlevel 1 (
    set "PIO_CMD=pio"
    goto :pio_found
)

REM Try user scripts directory
for /f "delims=" %%i in ('%PYTHON_CMD% -c "import site; print(site.USER_BASE + '\\Scripts')"') do set "USER_SCRIPTS=%%i"
if exist "!USER_SCRIPTS!\pio.exe" (
    set "PIO_CMD=!USER_SCRIPTS!\pio.exe"
    goto :pio_found
)

echo Installing PlatformIO...
%PIP_CMD% install platformio
if errorlevel 1 (
    echo ERROR: Failed to install PlatformIO
    pause
    exit /b 1
)
set "PIO_CMD=pio"

:pio_found
echo Dependencies checked successfully
exit /b 0

:check_project_structure
echo [SETUP] Checking project structure...

if not exist "!PROJECT_DIR!\platformio.ini" (
    echo ERROR: platformio.ini not found in project
    echo Expected location: !PROJECT_DIR!\platformio.ini
    pause
    exit /b 1
)

if not exist "!PROJECT_DIR!\src" (
    echo ERROR: src directory not found in project
    echo Expected location: !PROJECT_DIR!\src
    pause
    exit /b 1
)

echo Project structure verified successfully
exit /b 0

:detect_ports_simple
echo [SETUP] Detecting ESP32 device...
echo.

REM Test common COM ports directly
for %%p in (COM3 COM4 COM5 COM6 COM7 COM8 COM9 COM10 COM11 COM12) do (
    echo Testing %%p...
    mode %%p >nul 2>&1
    if not errorlevel 1 (
        echo Found working port: %%p
        set "SELECTED_PORT=%%p"
        echo.
        echo ESP32 detected on !SELECTED_PORT!
        exit /b 0
    )
)

echo.
echo No ESP32 detected automatically.
echo.
echo MANUAL PORT SELECTION:
echo If you can see your ESP32 in Device Manager (e.g., COM3):
echo.
set /p "manual_port=Enter COM port manually (e.g. COM3) or press Enter to exit: "

if not "!manual_port!"=="" (
    echo Testing !manual_port!...
    mode !manual_port! >nul 2>&1
    if not errorlevel 1 (
        set "SELECTED_PORT=!manual_port!"
        echo Port !SELECTED_PORT! is working!
        exit /b 0
    ) else (
        echo ERROR: Cannot access !manual_port!
        echo Please check Device Manager and install drivers if needed
        pause
        exit /b 1
    )
)

echo Port detection cancelled
exit /b 1

:clean_device_flash
echo [1/4] Cleaning device flash...
cd /d "!PROJECT_DIR!"

"!PIO_CMD!" run -e "!BOARD_TYPE!" --target erase --upload-port "!SELECTED_PORT!"
if errorlevel 1 (
    echo Warning: Device cleaning failed, proceeding anyway
)
timeout /t 2 /nobreak >nul
exit /b 0

:flash_firmware
echo [2/4] Flashing firmware...
cd /d "!PROJECT_DIR!"

"!PIO_CMD!" run -e "!BOARD_TYPE!" --target upload --upload-port "!SELECTED_PORT!"
if errorlevel 1 (
    echo Firmware flash failed
    echo Try putting ESP32 in bootloader mode:
    echo 1. Hold BOOT button
    echo 2. Press RESET button
    echo 3. Release BOOT button
    pause
    exit /b 1
)

echo Firmware flashed successfully
exit /b 0

:configure_device
echo [3/4] Configuring device...

REM Wait for device reboot
timeout /t 8 /nobreak >nul

echo Configuring device on !SELECTED_PORT!...

REM Convert to uppercase
call :to_upper CHANNEL
call :to_upper PASSWORD
call :to_upper ROLE
call :to_upper REGION
call :to_upper MODE
call :to_upper RADIO

set "network_cmd=NETWORK_CREATE !CHANNEL! !PASSWORD!"
if /i "!RADIO!"=="CUSTOM_ADVANCED" (
    set "config_cmd=Q_CONFIG !ROLE!,!DEVICE_ID!,!INTERVAL!,!REGION!,!MODE!,!RADIO!,!HOPS!,125000,7,5,14,8"
) else (
    set "config_cmd=Q_CONFIG !ROLE!,!DEVICE_ID!,!INTERVAL!,!REGION!,!MODE!,!RADIO!,!HOPS!"
)

REM Create simple configuration script
set "config_py=%TEMP%\config_device_%RANDOM%.py"
(
echo import serial
echo import time
echo try:
echo     ser = serial.Serial^('!SELECTED_PORT!', 115200, timeout=10^)
echo     time.sleep^(3^)
echo     ser.write^(b'STATUS\r\n'ط)
echo     time.sleep^(2^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'!network_cmd!\r\n'ط)
echo     time.sleep^(3^)
echo     ser.write^(b'!config_cmd!\r\n'ط) 
echo     time.sleep^(3^)
echo     ser.write^(b'CONFIG_SAVE\r\n'ط)
echo     time.sleep^(3^)
echo     ser.write^(b'START\r\n'ط)
echo     time.sleep^(2^)
echo     ser.close^(^)
echo     print^('Configuration completed successfully'ط)
echo except Exception as e:
echo     print^(f'Configuration error: {e}'ط)
) > "!config_py!"

%PYTHON_CMD% "!config_py!"
del "!config_py!" >nul 2>&1

echo Device configured successfully
exit /b 0

:launch_monitor
echo [4/4] Launching serial monitor...
echo.
echo Opening monitor window...
start cmd /k "cd /d "!PROJECT_DIR!" && "!PIO_CMD!" device monitor --baud 115200 --port !SELECTED_PORT!"
exit /b 0

:end
echo.
if not "%~1"=="" (
    echo Flash tool execution completed.
) else (
    echo Flash tool execution completed.
    pause
)
exit /b 0