
@echo off
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the Python flash tool
REM Automatically opens in Command Prompt when executed

REM Global variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Colors (Windows doesn't support colors in batch, so we'll use text formatting)
set "RED=[ERROR]"
set "GREEN=[OK]"
set "YELLOW=[WARN]"
set "BLUE=[INFO]"

REM Function to print colored output (simulated with text)
:print_color
set "color=%1"
set "message=%2"
echo %color% %message%
goto :eof

:print_header
cls
echo ==================================
echo     Custodia Flash Tool v2.0     
echo        Windows Command Version    
echo ==================================
echo.
goto :eof

REM Function to check if command exists
:command_exists
where %1 >nul 2>&1
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to install Python if needed
:install_python
call :command_exists python
if %errorlevel% neq 0 (
    call :command_exists python3
    if %errorlevel% neq 0 (
        call :print_color %YELLOW% "Python not found. Please install Python manually:"
        call :print_color %YELLOW% "1. Download from https://python.org"
        call :print_color %YELLOW% "2. Make sure to check 'Add Python to PATH' during installation"
        call :print_color %YELLOW% "3. Restart Command Prompt after installation"
        exit /b 1
    )
)
exit /b 0

REM Function to install dependencies
:install_dependencies
call :print_color %YELLOW% "[SETUP] Checking dependencies..."

REM Install Python if needed
call :install_python
if %errorlevel% neq 0 (
    exit /b 1
)

REM Check pip
call :command_exists pip
if %errorlevel% neq 0 (
    call :command_exists pip3
    if %errorlevel% neq 0 (
        call :print_color %RED% "ERROR: pip not found"
        call :print_color %YELLOW% "Try: python -m ensurepip --upgrade"
        exit /b 1
    )
    set "PIP_CMD=pip3"
) else (
    set "PIP_CMD=pip"
)

REM Check/install pyserial
python -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Installing pyserial..."
    %PIP_CMD% install pyserial
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "pyserial installed successfully"
    ) else (
        call :print_color %RED% "Failed to install pyserial"
        exit /b 1
    )
) else (
    call :print_color %GREEN% "pyserial already installed"
)

exit /b 0

REM Function to find PlatformIO
:find_platformio
call :command_exists pio
if %errorlevel% equ 0 (
    pio --version >nul 2>&1
    if %errorlevel% equ 0 (
        set "PIO_CMD=pio"
        exit /b 0
    )
)

REM Check common installation paths
set "POSSIBLE_PATHS=%USERPROFILE%\.local\bin\pio.exe;%USERPROFILE%\.platformio\penv\Scripts\pio.exe;%USERPROFILE%\AppData\Local\Programs\Python\Scripts\pio.exe"

for %%p in (%POSSIBLE_PATHS%) do (
    if exist "%%p" (
        "%%p" --version >nul 2>&1
        if !errorlevel! equ 0 (
            set "PIO_CMD=%%p"
            exit /b 0
        )
    )
)

exit /b 1

REM Function to install PlatformIO
:install_platformio
call :print_color %YELLOW% "PlatformIO not found. Installing globally..."

if %PIP_CMD% install --user platformio (
    call :print_color %GREEN% "PlatformIO installed successfully (user)"
    
    REM Add to PATH for current session
    set "PATH=%PATH%;%USERPROFILE%\.local\bin;%USERPROFILE%\AppData\Roaming\Python\Scripts"
    
    timeout /t 3 /nobreak >nul
    
    REM Try to find it again
    call :find_platformio
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "PlatformIO ready: %PIO_CMD%"
        exit /b 0
    ) else (
        call :print_color %YELLOW% "Installation completed but PlatformIO not immediately available"
        call :print_color %YELLOW% "Please restart Command Prompt and try again"
        exit /b 1
    )
) else (
    call :print_color %RED% "Failed to install PlatformIO"
    call :print_color %YELLOW% "Manual installation:"
    call :print_color %YELLOW% "1. %PIP_CMD% install --user platformio"
    call :print_color %YELLOW% "2. Add %%USERPROFILE%%\.local\bin to your PATH"
    exit /b 1
)

REM Function to verify project structure
:verify_project_structure
call :print_color %BLUE% "Verifying project structure..."

if not exist "%PROJECT_DIR%" (
    call :print_color %RED% "ERROR: Project directory not found: %PROJECT_DIR%"
    exit /b 1
)

if not exist "%PROJECT_DIR%\platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in project directory"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\platformio.ini"
    exit /b 1
)

if not exist "%PROJECT_DIR%\src" (
    call :print_color %RED% "ERROR: src directory not found in project"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\src"
    exit /b 1
)

call :print_color %GREEN% "Project structure verified successfully"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Script directory: %SCRIPT_DIR%"
exit /b 0

REM Function to detect ESP32 devices
:detect_ports
call :print_color %YELLOW% "Detecting ESP32 devices..."

REM Create Python script for port detection
set "PORT_SCRIPT=%TEMP%\detect_ports.py"
(
echo import serial.tools.list_ports
echo import sys
echo.
echo ports = serial.tools.list_ports.comports^(^)
echo esp32_ports = []
echo.
echo for port in ports:
echo     device_lower = port.device.lower^(^)
echo     desc_lower = port.description.lower^(^)
echo     manufacturer_lower = ^(port.manufacturer or ""^).lower^(^)
echo     
echo     # Skip non-relevant devices
echo     skip_patterns = ["bluetooth", "debug-console", "hub", "keyboard", "mouse"]
echo     if any^(pattern in desc_lower for pattern in skip_patterns^):
echo         continue
echo     
echo     # ESP32 indicators
echo     esp32_indicators = ["cp210", "ch340", "ch341", "ft232", "pl2303", "esp32", "serial", "uart", "usb-serial", "com"]
echo     
echo     is_esp32_candidate = False
echo     
echo     if any^(pattern in device_lower for pattern in ["com"]^):
echo         is_esp32_candidate = True
echo     
echo     if any^(indicator in desc_lower for indicator in esp32_indicators^):
echo         is_esp32_candidate = True
echo     
echo     if any^(mfg in manufacturer_lower for mfg in ["silicon labs", "ftdi", "prolific", "ch340"]^):
echo         is_esp32_candidate = True
echo     
echo     if is_esp32_candidate:
echo         esp32_ports.append^((port.device, port.description^)^)
echo.
echo if not esp32_ports:
echo     print^("NO_ESP32_FOUND"^)
echo     if ports:
echo         print^("AVAILABLE_PORTS:"^)
echo         for i, port in enumerate^(ports, 1^):
echo             print^(f"{i}:{port.device}:{port.description}"^)
echo else:
echo     if len^(esp32_ports^) == 1:
echo         print^(f"SINGLE_ESP32:{esp32_ports[0][0]}:{esp32_ports[0][1]}"^)
echo     else:
echo         print^("MULTIPLE_ESP32:"^)
echo         for i, ^(device, desc^) in enumerate^(esp32_ports, 1^):
echo             print^(f"{i}:{device}:{desc}"^)
) > "%PORT_SCRIPT%"

python "%PORT_SCRIPT%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Could not detect ports (pyserial issue)"
    del "%PORT_SCRIPT%" 2>nul
    exit /b 1
)

REM Parse the output
for /f "tokens=*" %%i in ('python "%PORT_SCRIPT%"') do (
    set "PORT_INFO=%%i"
    if "!PORT_INFO!"=="NO_ESP32_FOUND" (
        call :print_color %YELLOW% "No ESP32 devices detected automatically."
        del "%PORT_SCRIPT%" 2>nul
        exit /b 1
    ) else if "!PORT_INFO:~0,13!"=="SINGLE_ESP32:" (
        for /f "tokens=2,3 delims=:" %%a in ("!PORT_INFO!") do (
            set "SELECTED_PORT=%%a"
            call :print_color %GREEN% "ESP32 device detected: %%a - %%b"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        )
    ) else if "!PORT_INFO!"=="MULTIPLE_ESP32:" (
        call :print_color %YELLOW% "Multiple ESP32 devices found:"
        set "PORT_COUNT=0"
        for /f "tokens=*" %%j in ('python "%PORT_SCRIPT%"') do (
            set "LINE=%%j"
            if "!LINE:~0,1!" neq "M" (
                set /a PORT_COUNT+=1
                for /f "tokens=1,2,3 delims=:" %%a in ("!LINE!") do (
                    echo   %%a. %%b - %%c
                    set "PORT_!PORT_COUNT!=%%b"
                )
            )
        )
        echo.
        :select_port
        set /p "choice=Select port [1-%PORT_COUNT%]: "
        if "!choice!" geq "1" if "!choice!" leq "%PORT_COUNT%" (
            call set "SELECTED_PORT=%%PORT_!choice!%%"
            call :print_color %GREEN% "Selected: !SELECTED_PORT!"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        ) else (
            call :print_color %RED% "Invalid selection. Please enter a number between 1 and %PORT_COUNT%"
            goto select_port
        )
    )
)

del "%PORT_SCRIPT%" 2>nul
exit /b 1

REM Function to detect board type (only XIAO ESP32S3 now)
:detect_board_type
set "BOARD_TYPE=seeed_xiao_esp32s3"
call :print_color %BLUE% "Using board: XIAO ESP32S3"
exit /b 0

REM Function to validate role
:validate_role
set "role=%1"
set "role_upper=%role%"
call :to_upper role_upper

if "%role_upper%"=="TRACKER" exit /b 0
if "%role_upper%"=="RECEIVER" exit /b 0
if "%role_upper%"=="REPEATER" exit /b 0
exit /b 1

REM Function to get role with validation
:get_role
:role_loop
echo.
call :print_color %BLUE% "Device Role:"
echo   1. TRACKER  - GPS tracking device
echo   2. RECEIVER - Base station/receiver
echo   3. REPEATER - Signal repeater/relay
echo.
set /p "choice=Select role [1-3]: "

if "%choice%"=="1" (
    set "ROLE=TRACKER"
    goto :eof
) else if "%choice%"=="2" (
    set "ROLE=RECEIVER"
    goto :eof
) else if "%choice%"=="3" (
    set "ROLE=REPEATER"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto role_loop
)

REM Function to validate device ID
:validate_device_id
set "id=%1"
if %id% geq 1 if %id% leq 255 exit /b 0
exit /b 1

REM Function to get device ID with validation
:get_device_id
:id_loop
echo.
call :print_color %BLUE% "Device ID:"
echo   Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

call :validate_device_id %DEVICE_ID%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid device ID. Please enter a number between 1 and 255"
    goto id_loop
)

REM Function to validate interval
:validate_interval
set "interval=%1"
if %interval% geq 5 if %interval% leq 3600 exit /b 0
exit /b 1

REM Function to get interval with validation
:get_interval
:interval_loop
echo.
call :print_color %BLUE% "Transmission Interval:"
echo   How often device sends GPS updates (in seconds)
echo   Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

call :validate_interval %INTERVAL%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid interval. Please enter a number between 5 and 3600 seconds"
    goto interval_loop
)

REM Function to get region with validation
:get_region
:region_loop
echo.
call :print_color %BLUE% "LoRa Region:"
echo   1. US - United States
echo   2. EU - Europe
echo   3. CH - China
echo   4. AS - Asia
echo   5. JP - Japan
echo.
set /p "choice=Select region [1-5]: "

if "%choice%"=="1" (
    set "REGION=US"
    goto :eof
) else if "%choice%"=="2" (
    set "REGION=EU"
    goto :eof
) else if "%choice%"=="3" (
    set "REGION=CH"
    goto :eof
) else if "%choice%"=="4" (
    set "REGION=AS"
    goto :eof
) else if "%choice%"=="5" (
    set "REGION=JP"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, 3, 4, or 5"
    goto region_loop
)

REM Function to get mode with validation
:get_mode
:mode_loop
echo.
call :print_color %BLUE% "Operation Mode:"
echo   1. SIMPLE - Basic operation
echo   2. ADMIN  - Advanced features enabled
echo.
set /p "choice=Select mode [1-2]: "

if "%choice%"=="1" (
    set "MODE=SIMPLE"
    goto :eof
) else if "%choice%"=="2" (
    set "MODE=ADMIN"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1 or 2"
    goto mode_loop
)

REM Function to get custom advanced parameters
:get_custom_advanced_params
echo.
call :print_color %YELLOW% "Custom Advanced Radio Parameters:"
echo.

REM Bandwidth
:bw_loop
call :print_color %BLUE% "Bandwidth (kHz):"
echo   1. 125 kHz (default)
echo   2. 250 kHz
echo   3. 500 kHz
echo.
set /p "choice=Select bandwidth [1-3]: "

if "%choice%"=="1" (
    set "CUSTOM_BW=125"
    goto sf_loop
) else if "%choice%"=="2" (
    set "CUSTOM_BW=250"
    goto sf_loop
) else if "%choice%"=="3" (
    set "CUSTOM_BW=500"
    goto sf_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto bw_loop
)

REM Spreading Factor
:sf_loop
call :print_color %BLUE% "Spreading Factor:"
echo   1. SF7  (fastest, shortest range)
echo   2. SF8
echo   3. SF9
echo   4. SF10
echo   5. SF11
echo   6. SF12 (slowest, longest range)
echo.
set /p "choice=Select spreading factor [1-6]: "

if "%choice%"=="1" (
    set "CUSTOM_SF=7"
    goto cr_loop
) else if "%choice%"=="2" (
    set "CUSTOM_SF=8"
    goto cr_loop
) else if "%choice%"=="3" (
    set "CUSTOM_SF=9"
    goto cr_loop
) else if "%choice%"=="4" (
    set "CUSTOM_SF=10"
    goto cr_loop
) else if "%choice%"=="5" (
    set "CUSTOM_SF=11"
    goto cr_loop
) else if "%choice%"=="6" (
    set "CUSTOM_SF=12"
    goto cr_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-6"
    goto sf_loop
)

REM Coding Rate
:cr_loop
call :print_color %BLUE% "Coding Rate:"
echo   1. 4/5 (fastest)
echo   2. 4/6
echo   3. 4/7
echo   4. 4/8 (most robust)
echo.
set /p "choice=Select coding rate [1-4]: "

if "%choice%"=="1" (
    set "CUSTOM_CR=5"
    goto power_loop
) else if "%choice%"=="2" (
    set "CUSTOM_CR=6"
    goto power_loop
) else if "%choice%"=="3" (
    set "CUSTOM_CR=7"
    goto power_loop
) else if "%choice%"=="4" (
    set "CUSTOM_CR=8"
    goto power_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-4"
    goto cr_loop
)

REM TX Power
:power_loop
call :print_color %BLUE% "TX Power (dBm):"
echo   Range: 2-22 dBm
echo   Recommended: 14-20 dBm
echo.
set /p "CUSTOM_POWER=Enter TX power [2-22]: "

if %CUSTOM_POWER% geq 2 if %CUSTOM_POWER% leq 22 (
    goto preamble_loop
) else (
    call :print_color %RED% "Invalid power. Please enter a number between 2 and 22"
    goto power_loop
)

REM Preamble Length
:preamble_loop
call :print_color %BLUE% "Preamble Length:"
echo   Range: 6-65535
echo   Default: 8
echo.
set /p "CUSTOM_PREAMBLE=Enter preamble length [6-65535]: "

if %CUSTOM_PREAMBLE% geq 6 if %CUSTOM_PREAMBLE% leq 65535 (
    goto :eof
) else (
    call :print_color %RED% "Invalid preamble length. Please enter a number between 6 and 65535"
    goto preamble_loop
)

REM Function to get radio profile with validation
:get_radio
:radio_loop
echo.
call :print_color %BLUE% "Radio Profile:"
echo   1. DESERT_LONG_FAST  - Long range, fast transmission
echo   2. MOUNTAIN_STABLE   - Stable connection in mountains
echo   3. URBAN_DENSE       - Dense urban environments
echo   4. MESH_MAX_NODES    - Maximum mesh network nodes
echo   5. CUSTOM_ADVANCED   - Custom advanced settings
echo.
set /p "choice=Select radio profile [1-5]: "

if "%choice%"=="1" (
    set "RADIO=DESERT_LONG_FAST"
    goto :eof
) else if "%choice%"=="2" (
    set "RADIO=MOUNTAIN_STABLE"
    goto :eof
) else if "%choice%"=="3" (
    set "RADIO=URBAN_DENSE"
    goto :eof
) else if "%choice%"=="4" (
    set "RADIO=MESH_MAX_NODES"
    goto :eof
) else if "%choice%"=="5" (
    set "RADIO=CUSTOM_ADVANCED"
    call :get_custom_advanced_params
    call :print_color %YELLOW% "Note: Custom advanced parameters will be configured in the main tool"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-5"
    goto radio_loop
)

REM Function to validate hops
:validate_hops
set "hops=%1"
if %hops% geq 1 if %hops% leq 10 exit /b 0
exit /b 1

REM Function to get hops with validation
:get_hops
:hops_loop
echo.
call :print_color %BLUE% "Max Hops:"
echo   Maximum number of hops for mesh network (1-10)
echo   Default: 3
echo.
set /p "input=Enter max hops [1-10] or press Enter for default (3): "

if "%input%"=="" (
    set "HOPS=3"
    goto :eof
) else (
    call :validate_hops %input%
    if %errorlevel% equ 0 (
        set "HOPS=%input%"
        goto :eof
    ) else (
        call :print_color %RED% "Invalid hops. Please enter a number between 1 and 10"
        goto hops_loop
    )
)

REM Function to validate channel name
:validate_channel
set "channel=%1"
call :to_upper channel

REM Check length (simplified for batch)
if "%channel:~3%"=="" exit /b 1
if "%channel:~20%" neq "" exit /b 1

REM Check for reserved names
if "%channel%"=="CONFIG" exit /b 1
if "%channel%"=="ADMIN" exit /b 1
if "%channel%"=="DEBUG" exit /b 1
if "%channel%"=="SYSTEM" exit /b 1
if "%channel%"=="DEVICE" exit /b 1
if "%channel%"=="LORA" exit /b 1
if "%channel%"=="MESH" exit /b 1
if "%channel%"=="TEST" exit /b 1
if "%channel%"=="DEFAULT" exit /b 1

exit /b 0

REM Function to get channel with validation
:get_channel
:channel_loop
echo.
call :print_color %BLUE% "Network Channel:"
echo   Network name for your devices (3-20 characters)
echo   Only letters, numbers, and underscore allowed
echo   Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS
echo.
set /p "CHANNEL=Enter channel name: "

call :validate_channel "%CHANNEL%"
if %errorlevel% equ 0 (
    call :to_upper CHANNEL
    goto :eof
) else (
    call :print_color %RED% "Invalid channel name. Requirements:"
    call :print_color %RED% "  - 3-20 characters"
    call :print_color %RED% "  - Only letters, numbers, underscore"
    call :print_color %RED% "  - Cannot be: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT"
    goto channel_loop
)

REM Function to validate password
:validate_password
set "password=%1"
call :to_upper password

REM Check length (simplified for batch)
if "%password:~8%"=="" exit /b 1
if "%password:~32%" neq "" exit /b 1

REM Check if same as channel
if "%password%"=="%CHANNEL%" exit /b 1

exit /b 0

REM Function to generate password
:generate_password
set "chars=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
set "PASSWORD="

REM Generate 8-character password (simplified)
for /l %%i in (1,1,8) do (
    set /a "rand=!random! %% 36"
    call set "char=!chars:~!rand!,1!"
    set "PASSWORD=!PASSWORD!!char!"
)
goto :eof

REM Function to get password with validation
:get_password
:password_loop
echo.
call :print_color %BLUE% "Network Password:"
echo   Password requirements:
echo   - 8-32 characters
echo   - Must contain at least 1 number and 1 letter
echo   - Cannot be the same as channel name
echo   Examples: Secure123, MyPass99, Field2024
echo.
set /p "input=Enter password or press Enter for auto-generation: "

if "%input%"=="" (
    call :generate_password
    call :print_color %GREEN% "Password auto-generated: %PASSWORD%"
    goto :eof
) else (
    call :validate_password "%input%"
    if %errorlevel% equ 0 (
        set "PASSWORD=%input%"
        call :to_upper PASSWORD
        goto :eof
    ) else (
        call :print_color %RED% "Invalid password. Requirements:"
        call :print_color %RED% "  - 8-32 characters"
        call :print_color %RED% "  - At least 1 number and 1 letter"
        call :print_color %RED% "  - Cannot be same as channel name"
        goto password_loop
    )
)

REM Function to convert to uppercase
:to_upper
set "str=%1"
set "result="
for /f "delims=" %%i in ('cmd /c "echo %str%"') do (
    set "result=%%i"
)
set "%1=%result%"
goto :eof

REM Function to calculate network hash
:calculate_network_hash
set "combined=%CHANNEL%%PASSWORD%"
call :to_upper combined

REM Simple hash calculation (simplified for batch)
set "hash=0"
for /l %%i in (0,1,7) do (
    set "char=!combined:~%%i,1!"
    if defined char (
        set /a "hash=hash*31+!char!"
    )
)
set /a "hash=hash & 0xFFFFFFFF"
set "NETWORK_HASH=%hash%"
goto :eof

REM Function to clean device flash completely
:clean_device_flash
call :print_color %YELLOW% "[1/4] Cleaning device flash..."
call :print_color %BLUE% "Erasing all flash memory and configurations..."
call :print_color %BLUE% "This ensures no previous configurations interfere with new setup"

cd /d "%PROJECT_DIR%"
if %errorlevel% neq 0 (
    call :print_color %RED% "Cannot change to project directory: %PROJECT_DIR%"
    exit /b 1
)

if not exist "platformio.ini" (
    call :print_color %RED% "ERROR: Not in PlatformIO project directory"
    call :print_color %YELLOW% "Current directory: %CD%"
    exit /b 1
)

call :print_color %BLUE% "Attempting complete flash erase..."
"%PIO_CMD%" run -e %BOARD_TYPE% --target erase --upload-port %SELECTED_PORT%
if %errorlevel% equ 0 (
    call :print_color %GREEN% "Complete flash erase successful"
    timeout /t 3 /nobreak >nul
    exit /b 0
)

call :print_color %YELLOW% "Standard erase failed, trying esptool direct method..."

REM Try esptool directly
where esptool.py >nul 2>&1
if %errorlevel% equ 0 (
    call :print_color %BLUE% "Using esptool for thorough cleaning..."
    esptool.py --chip esp32s3 --port %SELECTED_PORT% erase_flash
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "Complete flash erase with esptool successful"
        timeout /t 3 /nobreak >nul
        exit /b 0
    )
)

call :print_color %YELLOW% "Warning: Could not perform complete flash erase"
call :print_color %YELLOW% "Device may retain some previous configurations"
call :print_color %YELLOW% "New configuration should still override old settings"
call :print_color %BLUE% "This is not critical - proceeding with firmware flash"

exit /b 0

REM Function to flash firmware
:flash_firmware
call :print_color %YELLOW% "[2/4] Flashing firmware..."

call :print_color %BLUE% "Debug: Script directory: %SCRIPT_DIR%"
call :print_color %BLUE% "Debug: Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Debug: PlatformIO command: %PIO_CMD%"

cd /d "%PROJECT_DIR%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Cannot access project directory: %PROJECT_DIR%"
    exit /b 1
)

if not exist "platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in %PROJECT_DIR%"
    call :print_color %YELLOW% "Current directory: %CD%"
    exit /b 1
)
call :print_color %GREEN% "Found platformio.ini in project directory"

call :detect_board_type
call :print_color %BLUE% "Using board: %BOARD_TYPE%"
call :print_color %BLUE% "Using environment: %BOARD_TYPE%"
call :print_color %BLUE% "Using port: %SELECTED_PORT%"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"

REM First clean the device
call :clean_device_flash
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Warning: Device cleaning failed, proceeding anyway"
)

timeout /t 2 /nobreak >nul

call :print_color %BLUE% "Executing: %PIO_CMD% run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%"
"%PIO_CMD%" run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%
if %errorlevel% equ 0 (
    call :print_color %GREEN% "Firmware flashed successfully"
    exit /b 0
) else (
    call :print_color %RED% "Firmware flash failed"
    call :print_color %YELLOW% "Debug info:"
    call :print_color %YELLOW% "- Current directory: %CD%"
    call :print_color %YELLOW% "- PlatformIO command: %PIO_CMD%"
    call :print_color %YELLOW% "- Board type: %BOARD_TYPE%"
    call :print_color %YELLOW% "- Port: %SELECTED_PORT%"
    call :print_color %YELLOW% ""
    call :print_color %YELLOW% "Manual solution:"
    call :print_color %YELLOW% "1. Close any open serial monitors"
    call :print_color %YELLOW% "2. Hold BOOT button"
    call :print_color %YELLOW% "3. Press and release RESET button"
    call :print_color %YELLOW% "4. Release BOOT button"
    call :print_color %YELLOW% "5. Manual cleaning commands (run from %PROJECT_DIR%):"
    call :print_color %YELLOW% "   cd \"%PROJECT_DIR%\""
    call :print_color %YELLOW% "   %PIO_CMD% run -e %BOARD_TYPE% --target erase --upload-port %SELECTED_PORT%"
    call :print_color %YELLOW% "   %PIO_CMD% run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%"
    exit /b 1
)

REM Function to configure device
:configure_device
call :print_color %YELLOW% "[3/4] Configuring device..."

timeout /t 8 /nobreak >nul

REM Re-detect port (may have changed after flash)
call :detect_ports
if %errorlevel% neq 0 (
    call :print_color %RED% "WARNING: Cannot detect port after flash"
    call :print_manual_config
    exit /b 1
)

call :print_color %BLUE% "Using configuration port: %SELECTED_PORT%"

REM Build configuration commands
call :to_upper CHANNEL
call :to_upper PASSWORD
call :to_upper ROLE
call :to_upper REGION
call :to_upper MODE
call :to_upper RADIO

set "network_cmd=NETWORK_CREATE %CHANNEL% %PASSWORD%"
set "config_cmd="

if "%RADIO%"=="CUSTOM_ADVANCED" (
    set "config_cmd=Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%,%CUSTOM_BW%,%CUSTOM_SF%,%CUSTOM_CR%,%CUSTOM_POWER%,%CUSTOM_PREAMBLE%"
) else (
    set "config_cmd=Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%"
)

set "config_cmd_alt=CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%"

REM Create Python script for serial communication
set "SERIAL_SCRIPT=%TEMP%\configure_device.py"
(
echo import serial
echo import time
echo import sys
echo.
echo try:
echo     ser = serial.Serial^('%SELECTED_PORT%', 115200, timeout=15^)
echo     time.sleep^(5^)  # Wait for device initialization
echo     
echo     print^('Clearing input buffer...'^)
echo     ser.reset_input_buffer^(^)
echo     
echo     # Test communication
echo     print^('Testing device communication...'^)
echo     ser.write^(b'\r\n'^)
echo     time.sleep^(1^)
echo     ser.write^(b'STATUS\r\n'^)
echo     time.sleep^(2^)
echo     
echo     # Clear status response
echo     while ser.in_waiting ^> 0:
echo         ser.read^(ser.in_waiting^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Device communication established'^)
echo     
echo     # Step 1: Create network
echo     print^('Creating network: %CHANNEL%'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(^('%network_cmd%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     
echo     network_response = ''
echo     while ser.in_waiting ^> 0:
echo         network_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Network response:', network_response[:200]^)
echo     
echo     # Step 2: Configure device
echo     print^('Configuring device parameters...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(^('%config_cmd%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     
echo     config_response = ''
echo     while ser.in_waiting ^> 0:
echo         config_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Configuration response:', config_response[:200]^)
echo     
echo     # Try alternative if needed
echo     if ^('comando desconocido' in config_response.lower^(^) or 
echo         'unknown command' in config_response.lower^(^) or
echo         len^(config_response.strip^(^)^) ^< 10^):
echo         
echo         print^('Trying alternative CONFIG command...'^)
echo         ser.reset_input_buffer^(^)
echo         ser.write^(^('%config_cmd_alt%' + '\r\n'^).encode^(^)^)
echo         time.sleep^(4^)
echo         
echo         while ser.in_waiting ^> 0:
echo             config_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo             time.sleep^(0.1^)
echo     
echo     # Step 3: Save configuration
echo     print^('Saving configuration to EEPROM...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'CONFIG_SAVE\r\n'^)
echo     time.sleep^(3^)
echo     
echo     save_response = ''
echo     while ser.in_waiting ^> 0:
echo         save_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Save response:', save_response[:100]^)
echo     
echo     # Step 4: Start device
echo     print^('Starting device operation...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'START\r\n'^)
echo     time.sleep^(2^)
echo     
echo     # Final status
echo     ser.write^(b'NETWORK_STATUS\r\n'^)
echo     time.sleep^(2^)
echo     
echo     final_response = ''
echo     while ser.in_waiting ^> 0:
echo         final_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     ser.close^(^)
echo     
echo     # Check for success
echo     all_responses = ^(network_response + config_response + save_response + final_response^).lower^(^)
echo     success_indicators = [
echo         'network creada exitosamente',
echo         'configuracion guardada exitosamente',
echo         'configuración guardada',
echo         'network activa',
echo         'listo y operando'
echo     ]
echo     
echo     if any^(indicator in all_responses for indicator in success_indicators^):
echo         print^('CONFIG_SUCCESS'^)
echo     else:
echo         print^('CONFIG_UNCERTAIN'^)
echo         
echo except Exception as e:
echo     print^('CONFIG_ERROR:', str^(e^)^)
) > "%SERIAL_SCRIPT%"

REM Execute configuration
python "%SERIAL_SCRIPT%" > "%TEMP%\config_result.txt" 2>&1
set "result="
for /f "tokens=*" %%i in ('type "%TEMP%\config_result.txt"') do (
    set "result=!result! %%i"
)

if "%result%"=="*CONFIG_SUCCESS*" (
    call :print_color %GREEN% "Configuration completed successfully"
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 0
) else if "%result%"=="*CONFIG_UNCERTAIN*" (
    call :print_color %YELLOW% "Configuration may have failed"
    call :print_manual_verification
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 0
) else (
    call :print_color %RED% "Configuration error"
    call :print_color %RED% "%result%"
    call :print_manual_config
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 1
)

REM Function to print manual configuration instructions
:print_manual_config
call :print_color %YELLOW% "=== MANUAL RECOVERY ==="
call :print_color %YELLOW% "1. Connect manually: %PIO_CMD% device monitor --port %SELECTED_PORT% --baud 115200"
call :print_color %YELLOW% "2. Send these commands one by one:"
call :print_color %YELLOW% "   STATUS"
call :print_color %YELLOW% "   NETWORK_CREATE %CHANNEL% %PASSWORD%"
if "%RADIO%"=="CUSTOM_ADVANCED" (
    call :print_color %YELLOW% "   Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%,%CUSTOM_BW%,%CUSTOM_SF%,%CUSTOM_CR%,%CUSTOM_POWER%,%CUSTOM_PREAMBLE%"
) else (
    call :print_color %YELLOW% "   Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%"
)
call :print_color %YELLOW% "   CONFIG_SAVE"
call :print_color %YELLOW% "   START"
call :print_color %YELLOW% "   NETWORK_STATUS"
exit /b 0

:print_manual_verification
call :print_color %YELLOW% "Manual verification required:"
call :print_color %YELLOW% "1. Connect to device: %PIO_CMD% device monitor --port %SELECTED_PORT% --baud 115200"
call :print_color %YELLOW% "2. Check status: STATUS"
call :print_color %YELLOW% "3. Check network: NETWORK_STATUS"
exit /b 0

REM Function to launch serial monitor
:launch_monitor
call :print_color %YELLOW% "[4/4] Launching serial monitor..."

REM Create a batch file to launch monitor in new window
set "MONITOR_SCRIPT=%TEMP%\custodia_monitor.bat"
(
echo @echo off
echo cd /d "%PROJECT_DIR%"
echo set "PATH=%%PATH%%;%%USERPROFILE%%\.local\bin;%%USERPROFILE%%\AppData\Roaming\Python\Scripts"
echo echo Custodia Device Monitor
echo echo ======================
echo echo Port: %SELECTED_PORT%
echo echo PlatformIO: %PIO_CMD%
echo echo.
echo "%PIO_CMD%" device monitor --baud 115200 --port %SELECTED_PORT%
echo echo.
echo echo Monitor session ended. Press Enter to close...
echo pause
echo del "%%~f0"
) > "%MONITOR_SCRIPT%"

REM Launch monitor in new Command Prompt window
start "Custodia Monitor" cmd /k "%MONITOR_SCRIPT%"
if %errorlevel% equ 0 (
    call :print_color %GREEN% "Monitor window opened successfully"
    call :print_color %BLUE% "Check your taskbar for the new Command Prompt window"
) else (
    call :print_color %YELLOW% "Could not auto-launch monitor window"
    call :print_color %YELLOW% "Manual command:"
    call :print_color %BLUE% "%PIO_CMD% device monitor --baud 115200 --port %SELECTED_PORT%"
)

call :print_color %BLUE% "Monitor command: %PIO_CMD% device monitor --baud 115200 --port %SELECTED_PORT%"
exit /b 0

REM Function to show final summary
:show_summary
call :print_color %BLUE% "===== CONFIGURATION SUMMARY ====="
call :print_color %GREEN% "Device ID: %DEVICE_ID%"
call :print_color %GREEN% "Role: %ROLE%"
call :print_color %GREEN% "Transmission Interval: %INTERVAL% seconds"
call :print_color %GREEN% "Region: %REGION%"
call :print_color %GREEN% "Mode: %MODE%"
call :print_color %GREEN% "Radio Profile: %RADIO%"
if "%RADIO%"=="CUSTOM_ADVANCED" (
    call :print_color %GREEN% "  Bandwidth: %CUSTOM_BW% kHz"
    call :print_color %GREEN% "  Spreading Factor: SF%CUSTOM_SF%"
    call :print_color %GREEN% "  Coding Rate: 4/%CUSTOM_CR%"
    call :print_color %GREEN% "  TX Power: %CUSTOM_POWER% dBm"
    call :print_color %GREEN% "  Preamble: %CUSTOM_PREAMBLE%"
)
call :print_color %GREEN% "Max Hops: %HOPS%"
call :print_color %GREEN% "Network Channel: %CHANNEL%"
call :print_color %GREEN% "Network Password: %PASSWORD%"
call :calculate_network_hash
call :print_color %GREEN% "Network Hash: %NETWORK_HASH%"
call :print_color %BLUE% "=================================="
echo.
call :print_color %BLUE% "STATUS: READY FOR OPERATION"
call :print_color %BLUE% "Monitor launched in separate Command Prompt window"
echo.
exit /b 0

REM Function to get configuration type
:get_config_type
:config_type_loop
echo.
call :print_color %BLUE% "Configuration Type:"
echo   1. Normal     - Interactive step-by-step configuration
echo   2. One Line   - Single command line configuration
echo.
set /p "choice=Select configuration type [1-2]: "

if "%choice%"=="1" (
    exit /b 0
) else if "%choice%"=="2" (
    exit /b 1
) else (
    call :print_color %RED% "Invalid selection. Please enter 1 or 2"
    goto config_type_loop
)

REM Function to validate all parameters
:validate_all_parameters
set "errors=0"

REM Validate role
call :validate_role "%ROLE%"
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid role: '%ROLE%'. Valid values: TRACKER, RECEIVER, REPEATER"
    set /a errors+=1
)

REM Validate device ID
call :validate_device_id %DEVICE_ID%
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid device ID: '%DEVICE_ID%'. Valid range: 1-255"
    set /a errors+=1
)

REM Validate interval
call :validate_interval %INTERVAL%
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid interval: '%INTERVAL%'. Valid range: 5-3600 seconds"
    set /a errors+=1
)

REM Validate region
if "%REGION%" neq "US" if "%REGION%" neq "EU" if "%REGION%" neq "CH" if "%REGION%" neq "AS" if "%REGION%" neq "JP" (
    call :print_color %RED% "Invalid region: '%REGION%'. Valid values: US, EU, CH, AS, JP"
    set /a errors+=1
)

REM Validate mode
if "%MODE%" neq "SIMPLE" if "%MODE%" neq "ADMIN" (
    call :print_color %RED% "Invalid mode: '%MODE%'. Valid values: SIMPLE, ADMIN"
    set /a errors+=1
)

REM Validate radio profile
if "%RADIO%" neq "DESERT_LONG_FAST" if "%RADIO%" neq "MOUNTAIN_STABLE" if "%RADIO%" neq "URBAN_DENSE" if "%RADIO%" neq "MESH_MAX_NODES" if "%RADIO%" neq "CUSTOM_ADVANCED" (
    call :print_color %RED% "Invalid radio profile: '%RADIO%'. Valid values: DESERT_LONG_FAST, MOUNTAIN_STABLE, URBAN_DENSE, MESH_MAX_NODES, CUSTOM_ADVANCED"
    set /a errors+=1
)

REM Validate hops
call :validate_hops %HOPS%
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid hops: '%HOPS%'. Valid range: 1-10"
    set /a errors+=1
)

REM Validate channel
call :validate_channel "%CHANNEL%"
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid channel name: '%CHANNEL%'"
    set /a errors+=1
)

REM Validate password
call :validate_password "%PASSWORD%"
if %errorlevel% neq 0 (
    call :print_color %RED% "Invalid password: '%PASSWORD%'"
    set /a errors+=1
)

if %errors% gtr 0 (
    echo.
    call :print_color %RED% "CONFIGURATION ERRORS FOUND"
    echo.
    exit /b 1
)

exit /b 0

REM Main function
:main
call :print_header

REM Check for help
if "%1"=="-h" goto show_help
if "%1"=="--help" goto show_help

REM If no parameters provided, enter interactive mode
if "%1"=="" (
    call :get_config_type
    if %errorlevel% equ 0 (
        REM Normal interactive mode
        call :print_color %GREEN% "Interactive Configuration Mode"
        call :get_role
        call :get_device_id
        call :get_interval
        call :get_region
        call :get_mode
        call :get_radio
        call :get_hops
        call :get_channel
        call :get_password
    ) else (
        REM One-line mode
        call :print_color %BLUE% "One-line Configuration Mode"
        echo.
        call :print_color %YELLOW% "Example:"
        call :print_color %GREEN% "flash_tool_windows.bat -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3"
        echo.
        call :print_color %YELLOW% "Available parameters:"
        call :print_color %BLUE% "  -role [TRACKER|RECEIVER|REPEATER]"
        call :print_color %BLUE% "  -id [1-255]"
        call :print_color %BLUE% "  -interval [5-3600]"
        call :print_color %BLUE% "  -region [US|EU|CH|AS|JP]"
        call :print_color %BLUE% "  -mode [SIMPLE|ADMIN]"
        call :print_color %BLUE% "  -radio [DESERT_LONG_FAST|MOUNTAIN_STABLE|URBAN_DENSE|MESH_MAX_NODES]"
        call :print_color %BLUE% "  -channel [channel_name]"
        call :print_color %BLUE% "  -password [password] (optional)"
        call :print_color %BLUE% "  -hops [1-10] (optional, default: 3)"
        echo.
        
        set /p "config_line=Enter your configuration command: "
        
        REM Parse the configuration line (simplified)
        for %%a in (%config_line%) do (
            if "%%a"=="-role" set "ROLE=%%b"
            if "%%a"=="-id" set "DEVICE_ID=%%b"
            if "%%a"=="-interval" set "INTERVAL=%%b"
            if "%%a"=="-region" set "REGION=%%b"
            if "%%a"=="-mode" set "MODE=%%b"
            if "%%a"=="-radio" set "RADIO=%%b"
            if "%%a"=="-hops" set "HOPS=%%b"
            if "%%a"=="-channel" set "CHANNEL=%%b"
            if "%%a"=="-password" set "PASSWORD=%%b"
        )
        
        REM Set defaults
        if "%HOPS%"=="" set "HOPS=3"
        
        REM Generate password if not provided
        if "%PASSWORD%"=="" (
            call :generate_password
            call :print_color %GREEN% "Password auto-generated: %PASSWORD%"
        )
        
        REM Validate parsed parameters
        call :validate_all_parameters
        if %errorlevel% neq 0 (
            exit /b 1
        )
    )
) else (
    REM Command line mode - parse arguments
    :parse_args
    if "%1"=="-role" (
        set "ROLE=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-id" (
        set "DEVICE_ID=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-interval" (
        set "INTERVAL=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-region" (
        set "REGION=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-mode" (
        set "MODE=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-radio" (
        set "RADIO=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-hops" (
        set "HOPS=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-channel" (
        set "CHANNEL=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-password" (
        set "PASSWORD=%2"
        shift
        shift
        goto parse_args
    )
    if "%1"=="-port" (
        set "SELECTED_PORT=%2"
        shift
        shift
        goto parse_args
    )
    if not "%1"=="" (
        call :print_color %RED% "Unknown parameter: %1"
        exit /b 1
    )
    
    REM Check required parameters for command line mode
    if "%ROLE%"=="" (
        call :print_color %RED% "Missing required parameter: -role"
        exit /b 1
    )
    if "%DEVICE_ID%"=="" (
        call :print_color %RED% "Missing required parameter: -id"
        exit /b 1
    )
    if "%INTERVAL%"=="" (
        call :print_color %RED% "Missing required parameter: -interval"
        exit /b 1
    )
    if "%REGION%"=="" (
        call :print_color %RED% "Missing required parameter: -region"
        exit /b 1
    )
    if "%MODE%"=="" (
        call :print_color %RED% "Missing required parameter: -mode"
        exit /b 1
    )
    if "%RADIO%"=="" (
        call :print_color %RED% "Missing required parameter: -radio"
        exit /b 1
    )
    if "%CHANNEL%"=="" (
        call :print_color %RED% "Missing required parameter: -channel"
        exit /b 1
    )
    
    REM Set defaults
    if "%HOPS%"=="" set "HOPS=3"
    
    REM Generate password if not provided
    if "%PASSWORD%"=="" (
        call :generate_password
        call :print_color %GREEN% "Password auto-generated: %PASSWORD%"
    )
    
    REM Validate all parameters
    call :validate_all_parameters
    if %errorlevel% neq 0 (
        exit /b 1
    )
)

call :print_color %GREEN% "All parameters valid"

REM Install dependencies
call :install_dependencies
if %errorlevel% neq 0 (
    call :print_color %RED% "System setup failed"
    exit /b 1
)

REM Verify project structure first
call :verify_project_structure
if %errorlevel% neq 0 (
    call :print_color %RED% "Project structure verification failed"
    exit /b 1
)

REM Find or install PlatformIO
call :print_color %YELLOW% "Checking PlatformIO installation..."
call :find_platformio
if %errorlevel% neq 0 (
    call :install_platformio
    if %errorlevel% neq 0 (
        exit /b 1
    )
) else (
    call :print_color %GREEN% "PlatformIO found: %PIO_CMD%"
)

REM Detect port if not provided
if "%SELECTED_PORT%"=="" (
    call :detect_ports
    if %errorlevel% neq 0 (
        call :print_color %RED% "Flash cancelled - No valid port selected"
        exit /b 1
    )
)

call :print_color %GREEN% "Using port: %SELECTED_PORT%"

REM Show configuration summary
call :print_color %YELLOW% "[CONFIG] Configuration Summary:"
call :print_color %BLUE% "Note: Device will be completely cleaned before flashing to ensure no previous configurations interfere"
call :print_color %BLUE% "Role: %ROLE%"
call :print_color %BLUE% "ID: %DEVICE_ID%"
call :print_color %BLUE% "Transmission interval: %INTERVAL% seconds"
call :print_color %BLUE% "Region: %REGION%"
call :print_color %BLUE% "Mode: %MODE%"
call :print_color %BLUE% "Radio: %RADIO%"
if "%RADIO%"=="CUSTOM_ADVANCED" (
    call :print_color %BLUE% "  Bandwidth: %CUSTOM_BW% kHz"
    call :print_color %BLUE% "  Spreading Factor: SF%CUSTOM_SF%"
    call :print_color %BLUE% "  Coding Rate: 4/%CUSTOM_CR%"
    call :print_color %BLUE% "  TX Power: %CUSTOM_POWER% dBm"
    call :print_color %BLUE% "  Preamble: %CUSTOM_PREAMBLE%"
)
call :print_color %BLUE% "Hops: %HOPS%"
call :print_color %BLUE% "Channel: %CHANNEL%"
call :print_color %BLUE% "Password: %PASSWORD%"
call :calculate_network_hash
call :print_color %BLUE% "Network hash: %NETWORK_HASH%"
echo.

REM Confirm before proceeding
set /p "confirm=Proceed with flashing? [Y/n]: "
if /i "%confirm%"=="n" (
    call :print_color %YELLOW% "Operation cancelled by user"
    exit /b 0
)

REM Execute main workflow
call :flash_firmware
if %errorlevel% equ 0 (
    call :configure_device
    if %errorlevel% equ 0 (
        call :show_summary
        call :launch_monitor
    ) else (
        call :print_color %RED% "Configuration failed"
        exit /b 1
    )
) else (
    call :print_color %RED% "Flash tool completed with errors"
    exit /b 1
)

exit /b 0

:show_help
echo Usage: %~nx0 [interactive mode]
echo        %~nx0 -role ROLE -id ID -interval SECONDS -region REGION -mode MODE -radio PROFILE -channel NAME [OPTIONS]
echo.
echo Interactive mode (no parameters): Guided configuration
echo Command line mode: Direct parameter specification
echo.
echo Examples:
echo   %~nx0  # Interactive mode
echo   %~nx0 -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA
exit /b 0

REM Make sure we're in the right directory
cd /d "%SCRIPT_DIR%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Cannot access script directory"
    exit /b 1
)

REM Run main function
call :main %*
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the Python flash tool
REM Automatically opens in Command Prompt when executed

REM Global variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Colors (Windows doesn't support colors in batch, so we'll use text formatting)
set "RED=[ERROR]"
set "GREEN=[OK]"
set "YELLOW=[WARN]"
set "BLUE=[INFO]"

REM Function to print colored output (simulated with text)
:print_color
set "color=%1"
set "message=%2"
echo %color% %message%
goto :eof

:print_header
cls
echo ==================================
echo     Custodia Flash Tool v2.0     
echo        Windows Command Version    
echo ==================================
echo.
goto :eof

REM Function to check if command exists
:command_exists
where %1 >nul 2>&1
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to install Python if needed
:install_python
call :command_exists python
if %errorlevel% neq 0 (
    call :command_exists python3
    if %errorlevel% neq 0 (
        call :print_color %YELLOW% "Python not found. Please install Python manually:"
        call :print_color %YELLOW% "1. Download from https://python.org"
        call :print_color %YELLOW% "2. Make sure to check 'Add Python to PATH' during installation"
        call :print_color %YELLOW% "3. Restart Command Prompt after installation"
        exit /b 1
    )
)
exit /b 0

REM Function to install dependencies
:install_dependencies
call :print_color %YELLOW% "[SETUP] Checking dependencies..."

REM Install Python if needed
call :install_python
if %errorlevel% neq 0 (
    exit /b 1
)

REM Check pip
call :command_exists pip
if %errorlevel% neq 0 (
    call :command_exists pip3
    if %errorlevel% neq 0 (
        call :print_color %RED% "ERROR: pip not found"
        call :print_color %YELLOW% "Try: python -m ensurepip --upgrade"
        exit /b 1
    )
    set "PIP_CMD=pip3"
) else (
    set "PIP_CMD=pip"
)

REM Check/install pyserial
python -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Installing pyserial..."
    %PIP_CMD% install pyserial
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "pyserial installed successfully"
    ) else (
        call :print_color %RED% "Failed to install pyserial"
        exit /b 1
    )
) else (
    call :print_color %GREEN% "pyserial already installed"
)

exit /b 0

REM Function to find PlatformIO
:find_platformio
call :command_exists pio
if %errorlevel% equ 0 (
    pio --version >nul 2>&1
    if %errorlevel% equ 0 (
        set "PIO_CMD=pio"
        exit /b 0
    )
)

REM Check common installation paths
set "POSSIBLE_PATHS=%USERPROFILE%\.local\bin\pio.exe;%USERPROFILE%\.platformio\penv\Scripts\pio.exe;%USERPROFILE%\AppData\Local\Programs\Python\Scripts\pio.exe"

for %%p in (%POSSIBLE_PATHS%) do (
    if exist "%%p" (
        "%%p" --version >nul 2>&1
        if !errorlevel! equ 0 (
            set "PIO_CMD=%%p"
            exit /b 0
        )
    )
)

exit /b 1

REM Function to install PlatformIO
:install_platformio
call :print_color %YELLOW% "PlatformIO not found. Installing globally..."

if %PIP_CMD% install --user platformio (
    call :print_color %GREEN% "PlatformIO installed successfully (user)"
    
    REM Add to PATH for current session
    set "PATH=%PATH%;%USERPROFILE%\.local\bin;%USERPROFILE%\AppData\Roaming\Python\Scripts"
    
    timeout /t 3 /nobreak >nul
    
    REM Try to find it again
    call :find_platformio
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "PlatformIO ready: %PIO_CMD%"
        exit /b 0
    ) else (
        call :print_color %YELLOW% "Installation completed but PlatformIO not immediately available"
        call :print_color %YELLOW% "Please restart Command Prompt and try again"
        exit /b 1
    )
) else (
    call :print_color %RED% "Failed to install PlatformIO"
    call :print_color %YELLOW% "Manual installation:"
    call :print_color %YELLOW% "1. %PIP_CMD% install --user platformio"
    call :print_color %YELLOW% "2. Add %%USERPROFILE%%\.local\bin to your PATH"
    exit /b 1
)

REM Function to verify project structure
:verify_project_structure
call :print_color %BLUE% "Verifying project structure..."

if not exist "%PROJECT_DIR%" (
    call :print_color %RED% "ERROR: Project directory not found: %PROJECT_DIR%"
    exit /b 1
)

if not exist "%PROJECT_DIR%\platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in project directory"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\platformio.ini"
    exit /b 1
)

if not exist "%PROJECT_DIR%\src" (
    call :print_color %RED% "ERROR: src directory not found in project"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\src"
    exit /b 1
)

call :print_color %GREEN% "Project structure verified successfully"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Script directory: %SCRIPT_DIR%"
exit /b 0

REM Function to detect ESP32 devices
:detect_ports
call :print_color %YELLOW% "Detecting ESP32 devices..."

REM Create Python script for port detection
set "PORT_SCRIPT=%TEMP%\detect_ports.py"
(
echo import serial.tools.list_ports
echo import sys
echo.
echo ports = serial.tools.list_ports.comports^(^)
echo esp32_ports = []
echo.
echo for port in ports:
echo     device_lower = port.device.lower^(^)
echo     desc_lower = port.description.lower^(^)
echo     manufacturer_lower = ^(port.manufacturer or ""^).lower^(^)
echo     
echo     # Skip non-relevant devices
echo     skip_patterns = ["bluetooth", "debug-console", "hub", "keyboard", "mouse"]
echo     if any^(pattern in desc_lower for pattern in skip_patterns^):
echo         continue
echo     
echo     # ESP32 indicators
echo     esp32_indicators = ["cp210", "ch340", "ch341", "ft232", "pl2303", "esp32", "serial", "uart", "usb-serial", "com"]
echo     
echo     is_esp32_candidate = False
echo     
echo     if any^(pattern in device_lower for pattern in ["com"]^):
echo         is_esp32_candidate = True
echo     
echo     if any^(indicator in desc_lower for indicator in esp32_indicators^):
echo         is_esp32_candidate = True
echo     
echo     if any^(mfg in manufacturer_lower for mfg in ["silicon labs", "ftdi", "prolific", "ch340"]^):
echo         is_esp32_candidate = True
echo     
echo     if is_esp32_candidate:
echo         esp32_ports.append^((port.device, port.description^)^)
echo.
echo if not esp32_ports:
echo     print^("NO_ESP32_FOUND"^)
echo     if ports:
echo         print^("AVAILABLE_PORTS:"^)
echo         for i, port in enumerate^(ports, 1^):
echo             print^(f"{i}:{port.device}:{port.description}"^)
echo else:
echo     if len^(esp32_ports^) == 1:
echo         print^(f"SINGLE_ESP32:{esp32_ports[0][0]}:{esp32_ports[0][1]}"^)
echo     else:
echo         print^("MULTIPLE_ESP32:"^)
echo         for i, ^(device, desc^) in enumerate^(esp32_ports, 1^):
echo             print^(f"{i}:{device}:{desc}"^)
) > "%PORT_SCRIPT%"

python "%PORT_SCRIPT%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Could not detect ports (pyserial issue)"
    del "%PORT_SCRIPT%" 2>nul
    exit /b 1
)

REM Parse the output
for /f "tokens=*" %%i in ('python "%PORT_SCRIPT%"') do (
    set "PORT_INFO=%%i"
    if "!PORT_INFO!"=="NO_ESP32_FOUND" (
        call :print_color %YELLOW% "No ESP32 devices detected automatically."
        del "%PORT_SCRIPT%" 2>nul
        exit /b 1
    ) else if "!PORT_INFO:~0,13!"=="SINGLE_ESP32:" (
        for /f "tokens=2,3 delims=:" %%a in ("!PORT_INFO!") do (
            set "SELECTED_PORT=%%a"
            call :print_color %GREEN% "ESP32 device detected: %%a - %%b"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        )
    ) else if "!PORT_INFO!"=="MULTIPLE_ESP32:" (
        call :print_color %YELLOW% "Multiple ESP32 devices found:"
        set "PORT_COUNT=0"
        for /f "tokens=*" %%j in ('python "%PORT_SCRIPT%"') do (
            set "LINE=%%j"
            if "!LINE:~0,1!" neq "M" (
                set /a PORT_COUNT+=1
                for /f "tokens=1,2,3 delims=:" %%a in ("!LINE!") do (
                    echo   %%a. %%b - %%c
                    set "PORT_!PORT_COUNT!=%%b"
                )
            )
        )
        echo.
        :select_port
        set /p "choice=Select port [1-%PORT_COUNT%]: "
        if "!choice!" geq "1" if "!choice!" leq "%PORT_COUNT%" (
            call set "SELECTED_PORT=%%PORT_!choice!%%"
            call :print_color %GREEN% "Selected: !SELECTED_PORT!"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        ) else (
            call :print_color %RED% "Invalid selection. Please enter a number between 1 and %PORT_COUNT%"
            goto select_port
        )
    )
)

del "%PORT_SCRIPT%" 2>nul
exit /b 1

REM Function to detect board type (only XIAO ESP32S3 now)
:detect_board_type
set "BOARD_TYPE=seeed_xiao_esp32s3"
call :print_color %BLUE% "Using board: XIAO ESP32S3"
exit /b 0

REM Function to validate role
:validate_role
set "role=%1"
set "role_upper=%role%"
call :to_upper role_upper

if "%role_upper%"=="TRACKER" exit /b 0
if "%role_upper%"=="RECEIVER" exit /b 0
if "%role_upper%"=="REPEATER" exit /b 0
exit /b 1

REM Function to get role with validation
:get_role
:role_loop
echo.
call :print_color %BLUE% "Device Role:"
echo   1. TRACKER  - GPS tracking device
echo   2. RECEIVER - Base station/receiver
echo   3. REPEATER - Signal repeater/relay
echo.
set /p "choice=Select role [1-3]: "

if "%choice%"=="1" (
    set "ROLE=TRACKER"
    goto :eof
) else if "%choice%"=="2" (
    set "ROLE=RECEIVER"
    goto :eof
) else if "%choice%"=="3" (
    set "ROLE=REPEATER"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto role_loop
)

REM Function to validate device ID
:validate_device_id
set "id=%1"
if %id% geq 1 if %id% leq 255 exit /b 0
exit /b 1

REM Function to get device ID with validation
:get_device_id
:id_loop
echo.
call :print_color %BLUE% "Device ID:"
echo   Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

call :validate_device_id %DEVICE_ID%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid device ID. Please enter a number between 1 and 255"
    goto id_loop
)

REM Function to validate interval
:validate_interval
set "interval=%1"
if %interval% geq 5 if %interval% leq 3600 exit /b 0
exit /b 1

REM Function to get interval with validation
:get_interval
:interval_loop
echo.
call :print_color %BLUE% "Transmission Interval:"
echo   How often device sends GPS updates (in seconds)
echo   Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

call :validate_interval %INTERVAL%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid interval. Please enter a number between 5 and 3600 seconds"
    goto interval_loop
)

REM Function to get region with validation
:get_region
:region_loop
echo.
call :print_color %BLUE% "LoRa Region:"
echo   1. US - United States
echo   2. EU - Europe
echo   3. CH - China
echo   4. AS - Asia
echo   5. JP - Japan
echo.
set /p "choice=Select region [1-5]: "

if "%choice%"=="1" (
    set "REGION=US"
    goto :eof
) else if "%choice%"=="2" (
    set "REGION=EU"
    goto :eof
) else if "%choice%"=="3" (
    set "REGION=CH"
    goto :eof
) else if "%choice%"=="4" (
    set "REGION=AS"
    goto :eof
) else if "%choice%"=="5" (
    set "REGION=JP"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, 3, 4, or 5"
    goto region_loop
)

REM Function to get mode with validation
:get_mode
:mode_loop
echo.
call :print_color %BLUE% "Operation Mode:"
echo   1. SIMPLE - Basic operation
echo   2. ADMIN  - Advanced features enabled
echo.
set /p "choice=Select mode [1-2]: "

if "%choice%"=="1" (
    set "MODE=SIMPLE"
    goto :eof
) else if "%choice%"=="2" (
    set "MODE=ADMIN"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1 or 2"
    goto mode_loop
)

REM Function to get custom advanced parameters
:get_custom_advanced_params
echo.
call :print_color %YELLOW% "Custom Advanced Radio Parameters:"
echo.

REM Bandwidth
:bw_loop
call :print_color %BLUE% "Bandwidth (kHz):"
echo   1. 125 kHz (default)
echo   2. 250 kHz
echo   3. 500 kHz
echo.
set /p "choice=Select bandwidth [1-3]: "

if "%choice%"=="1" (
    set "CUSTOM_BW=125"
    goto sf_loop
) else if "%choice%"=="2" (
    set "CUSTOM_BW=250"
    goto sf_loop
) else if "%choice%"=="3" (
    set "CUSTOM_BW=500"
    goto sf_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto bw_loop
)

REM Spreading Factor
:sf_loop
call :print_color %BLUE% "Spreading Factor:"
echo   1. SF7  (fastest, shortest range)
echo   2. SF8
echo   3. SF9
echo   4. SF10
echo   5. SF11
echo   6. SF12 (slowest, longest range)
echo.
set /p "choice=Select spreading factor [1-6]: "

if "%choice%"=="1" (
    set "CUSTOM_SF=7"
    goto cr_loop
) else if "%choice%"=="2" (
    set "CUSTOM_SF=8"
    goto cr_loop
) else if "%choice%"=="3" (
    set "CUSTOM_SF=9"
    goto cr_loop
) else if "%choice%"=="4" (
    set "CUSTOM_SF=10"
    goto cr_loop
) else if "%choice%"=="5" (
    set "CUSTOM_SF=11"
    goto cr_loop
) else if "%choice%"=="6" (
    set "CUSTOM_SF=12"
    goto cr_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-6"
    goto sf_loop
)

REM Coding Rate
:cr_loop
call :print_color %BLUE% "Coding Rate:"
echo   1. 4/5 (fastest)
echo   2. 4/6
echo   3. 4/7
echo   4. 4/8 (most robust)
echo.
set /p "choice=Select coding rate [1-4]: "

if "%choice%"=="1" (
    set "CUSTOM_CR=5"
    goto power_loop
) else if "%choice%"=="2" (
    set "CUSTOM_CR=6"
    goto power_loop
) else if "%choice%"=="3" (
    set "CUSTOM_CR=7"
    goto power_loop
) else if "%choice%"=="4" (
    set "CUSTOM_CR=8"
    goto power_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-4"
    goto cr_loop
)

REM TX Power
:power_loop
call :print_color %BLUE% "TX Power (dBm):"
echo   Range: 2-22 dBm
echo   Recommended: 14-20 dBm
echo.
set /p "CUSTOM_POWER=Enter TX power [2-22]: "

if %CUSTOM_POWER% geq 2 if %CUSTOM_POWER% leq 22 (
    goto preamble_loop
) else (
    call :print_color %RED% "Invalid power. Please enter a number between 2 and 22"
    goto power_loop
)

REM Preamble Length
:preamble_loop
call :print_color %BLUE% "Preamble Length:"
echo   Range: 6-65535
echo   Default: 8
echo.
set /p "CUSTOM_PREAMBLE=Enter preamble length [6-65535]: "

if %CUSTOM_PREAMBLE% geq 6 if %CUSTOM_PREAMBLE% leq 65535 (
    goto :eof
) else (
    call :print_color %RED% "Invalid preamble length. Please enter a number between 6 and 65535"
    goto preamble_loop
)

REM Function to get radio profile with validation
:get_radio
:radio_loop
echo.
call :print_color %BLUE% "Radio Profile:"
echo   1. DESERT_LONG_FAST  - Long range, fast transmission
echo   2. MOUNTAIN_STABLE   - Stable connection in mountains
echo   3. URBAN_DENSE       - Dense urban environments
echo   4. MESH_MAX_NODES    - Maximum mesh network nodes
echo   5. CUSTOM_ADVANCED   - Custom advanced settings
echo.
set /p "choice=Select radio profile [1-5]: "

if "%choice%"=="1" (
    set "RADIO=DESERT_LONG_FAST"
    goto :eof
) else if "%choice%"=="2" (
    set "RADIO=MOUNTAIN_STABLE"
    goto :eof
) else if "%choice%"=="3" (
    set "RADIO=URBAN_DENSE"
    goto :eof
) else if "%choice%"=="4" (
    set "RADIO=MESH_MAX_NODES"
    goto :eof
) else if "%choice%"=="5" (
    set "RADIO=CUSTOM_ADVANCED"
    call :get_custom_advanced_params
    call :print_color %YELLOW% "Note: Custom advanced parameters will be configured in the main tool"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-5"
    goto radio_loop
)

REM Function to validate hops
:validate_hops
set "hops=%1"
if %hops% geq 1 if %hops% leq 10 exit /b 0
exit /b 1

REM Function to get hops with validation
:get_hops
:hops_loop
echo.
call :print_color %BLUE% "Max Hops:"
echo   Maximum number of hops for mesh network (1-10)
echo   Default: 3
echo.
set /p "input=Enter max hops [1-10] or press Enter for default (3): "

if "%input%"=="" (
    set "HOPS=3"
    goto :eof
) else (
    call :validate_hops %input%
    if %errorlevel% equ 0 (
        set "HOPS=%input%"
        goto :eof
    ) else (
        call :print_color %RED% "Invalid hops. Please enter a number between 1 and 10"
        goto hops_loop
    )
)

REM Function to validate channel name
:validate_channel
set "channel=%1"
call :to_upper channel

REM Check length (simplified for batch)
if "%channel:~3%"=="" exit /b 1
if "%channel:~20%" neq "" exit /b 1

REM Check for reserved names
if "%channel%"=="CONFIG" exit /b 1
if "%channel%"=="ADMIN" exit /b 1
if "%channel%"=="DEBUG" exit /b 1
if "%channel%"=="SYSTEM" exit /b 1
if "%channel%"=="DEVICE" exit /b 1
if "%channel%"=="LORA" exit /b 1
if "%channel%"=="MESH" exit /b 1
if "%channel%"=="TEST" exit /b 1
if "%channel%"=="DEFAULT" exit /b 1

exit /b 0

REM Function to get channel with validation
:get_channel
:channel_loop
echo.
call :print_color %BLUE% "Network Channel:"
echo   Network name for your devices (3-20 characters)
echo   Only letters, numbers, and underscore allowed
echo   Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS
echo.
set /p "CHANNEL=Enter channel name: "

call :validate_channel "%CHANNEL%"
if %errorlevel% equ 0 (
    call :to_upper CHANNEL
    goto :eof
) else (
    call :print_color %RED% "Invalid channel name. Requirements:"
    call :print_color %RED% "  - 3-20 characters"
    call :print_color %RED% "  - Only letters, numbers, underscore"
    call :print_color %RED% "  - Cannot be: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT"
    goto channel_loop
)

REM Function to validate password
:validate_password
set "password=%1"
call :to_upper password

REM Check length (simplified for batch)
if "%password:~8%"=="" exit /b 1
if "%password:~32%" neq "" exit /b 1

REM Check if same as channel
if "%password%"=="%CHANNEL%" exit /b 1

exit /b 0

REM Function to generate password
:generate_password
set "chars=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
set "PASSWORD="

REM Generate 8-character password (simplified)
for /l %%i in (1,1,8) do (
    set /a "rand=!random! %% 36"
    call set "char=!chars:~!rand!,1!"
    set "PASSWORD=!PASSWORD!!char!"
)
goto :eof

REM Function to get password with validation
:get_password
:password_loop
echo.
call :print_color %BLUE% "Network Password:"
echo   Password requirements:
echo   - 8-32 characters
echo   - Must contain at least 1 number and 1 letter
echo   - Cannot be the same as channel name
echo   Examples: Secure123, MyPass99, Field2024
echo.
set /p "input=Enter password or press Enter for auto-generation: "

if "%input%"=="" (
    call :generate_password
    call :print_color %GREEN% "Password auto-generated: %PASSWORD%"
    goto :eof
) else (
    call :validate_password "%input%"
    if %errorlevel% equ 0 (
        set "PASSWORD=%input%"
        call :to_upper PASSWORD
        goto :eof
    ) else (
        call :print_color %RED% "Invalid password. Requirements:"
        call :print_color %RED% "  - 8-32 characters"
        call :print_color %RED% "  - At least 1 number and 1 letter"
        call :print_color %RED% "  - Cannot be same as channel name"
        goto password_loop
    )
)

REM Function to convert to uppercase
:to_upper
set "str=%1"
set "result="
for /f "delims=" %%i in ('cmd /c "echo %str%"') do (
    set "result=%%i"
)
set "%1=%result%"
goto :eof

REM Function to calculate network hash
:calculate_network_hash
set "combined=%CHANNEL%%PASSWORD%"
call :to_upper combined

REM Simple hash calculation (simplified for batch)
set "hash=0"
for /l %%i in (0,1,7) do (
    set "char=!combined:~%%i,1!"
    if defined char (
        set /a "hash=hash*31+!char!"
    )
)
set /a "hash=hash & 0xFFFFFFFF"
set "NETWORK_HASH=%hash%"
goto :eof

REM Function to clean device flash completely
:clean_device_flash
call :print_color %YELLOW% "[1/4] Cleaning device flash..."
call :print_color %BLUE% "Erasing all flash memory and configurations..."
call :print_color %BLUE% "This ensures no previous configurations interfere with new setup"

cd /d "%PROJECT_DIR%"
if %errorlevel% neq 0 (
    call :print_color %RED% "Cannot change to project directory: %PROJECT_DIR%"
    exit /b 1
)

if not exist "platformio.ini" (
    call :print_color %RED% "ERROR: Not in PlatformIO project directory"
    call :print_color %YELLOW% "Current directory: %CD%"
    exit /b 1
)

call :print_color %BLUE% "Attempting complete flash erase..."
"%PIO_CMD%" run -e %BOARD_TYPE% --target erase --upload-port %SELECTED_PORT%
if %errorlevel% equ 0 (
    call :print_color %GREEN% "Complete flash erase successful"
    timeout /t 3 /nobreak >nul
    exit /b 0
)

call :print_color %YELLOW% "Standard erase failed, trying esptool direct method..."

REM Try esptool directly
where esptool.py >nul 2>&1
if %errorlevel% equ 0 (
    call :print_color %BLUE% "Using esptool for thorough cleaning..."
    esptool.py --chip esp32s3 --port %SELECTED_PORT% erase_flash
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "Complete flash erase with esptool successful"
        timeout /t 3 /nobreak >nul
        exit /b 0
    )
)

call :print_color %YELLOW% "Warning: Could not perform complete flash erase"
call :print_color %YELLOW% "Device may retain some previous configurations"
call :print_color %YELLOW% "New configuration should still override old settings"
call :print_color %BLUE% "This is not critical - proceeding with firmware flash"

exit /b 0

REM Function to flash firmware
:flash_firmware
call :print_color %YELLOW% "[2/4] Flashing firmware..."

call :print_color %BLUE% "Debug: Script directory: %SCRIPT_DIR%"
call :print_color %BLUE% "Debug: Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Debug: PlatformIO command: %PIO_CMD%"

cd /d "%PROJECT_DIR%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Cannot access project directory: %PROJECT_DIR%"
    exit /b 1
)

if not exist "platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in %PROJECT_DIR%"
    call :print_color %YELLOW% "Current directory: %CD%"
    exit /b 1
)
call :print_color %GREEN% "Found platformio.ini in project directory"

call :detect_board_type
call :print_color %BLUE% "Using board: %BOARD_TYPE%"
call :print_color %BLUE% "Using environment: %BOARD_TYPE%"
call :print_color %BLUE% "Using port: %SELECTED_PORT%"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"

REM First clean the device
call :clean_device_flash
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Warning: Device cleaning failed, proceeding anyway"
)

timeout /t 2 /nobreak >nul

call :print_color %BLUE% "Executing: %PIO_CMD% run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%"
"%PIO_CMD%" run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%
if %errorlevel% equ 0 (
    call :print_color %GREEN% "Firmware flashed successfully"
    exit /b 0
) else (
    call :print_color %RED% "Firmware flash failed"
    call :print_color %YELLOW% "Debug info:"
    call :print_color %YELLOW% "- Current directory: %CD%"
    call :print_color %YELLOW% "- PlatformIO command: %PIO_CMD%"
    call :print_color %YELLOW% "- Board type: %BOARD_TYPE%"
    call :print_color %YELLOW% "- Port: %SELECTED_PORT%"
    call :print_color %YELLOW% ""
    call :print_color %YELLOW% "Manual solution:"
    call :print_color %YELLOW% "1. Close any open serial monitors"
    call :print_color %YELLOW% "2. Hold BOOT button"
    call :print_color %YELLOW% "3. Press and release RESET button"
    call :print_color %YELLOW% "4. Release BOOT button"
    call :print_color %YELLOW% "5. Manual cleaning commands (run from %PROJECT_DIR%):"
    call :print_color %YELLOW% "   cd \"%PROJECT_DIR%\""
    call :print_color %YELLOW% "   %PIO_CMD% run -e %BOARD_TYPE% --target erase --upload-port %SELECTED_PORT%"
    call :print_color %YELLOW% "   %PIO_CMD% run -e %BOARD_TYPE% --target upload --upload-port %SELECTED_PORT%"
    exit /b 1
)

REM Function to configure device
:configure_device
call :print_color %YELLOW% "[3/4] Configuring device..."

timeout /t 8 /nobreak >nul

REM Re-detect port (may have changed after flash)
call :detect_ports
if %errorlevel% neq 0 (
    call :print_color %RED% "WARNING: Cannot detect port after flash"
    call :print_manual_config
    exit /b 1
)

call :print_color %BLUE% "Using configuration port: %SELECTED_PORT%"

REM Build configuration commands
call :to_upper CHANNEL
call :to_upper PASSWORD
call :to_upper ROLE
call :to_upper REGION
call :to_upper MODE
call :to_upper RADIO

set "network_cmd=NETWORK_CREATE %CHANNEL% %PASSWORD%"
set "config_cmd="

if "%RADIO%"=="CUSTOM_ADVANCED" (
    set "config_cmd=Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%,%CUSTOM_BW%,%CUSTOM_SF%,%CUSTOM_CR%,%CUSTOM_POWER%,%CUSTOM_PREAMBLE%"
) else (
    set "config_cmd=Q_CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%"
)

set "config_cmd_alt=CONFIG %ROLE%,%DEVICE_ID%,%INTERVAL%,%REGION%,%MODE%,%RADIO%,%HOPS%"

REM Create Python script for serial communication
set "SERIAL_SCRIPT=%TEMP%\configure_device.py"
(
echo import serial
echo import time
echo import sys
echo.
echo try:
echo     ser = serial.Serial^('%SELECTED_PORT%', 115200, timeout=15^)
echo     time.sleep^(5^)  # Wait for device initialization
echo     
echo     print^('Clearing input buffer...'^)
echo     ser.reset_input_buffer^(^)
echo     
echo     # Test communication
echo     print^('Testing device communication...'^)
echo     ser.write^(b'\r\n'^)
echo     time.sleep^(1^)
echo     ser.write^(b'STATUS\r\n'^)
echo     time.sleep^(2^)
echo     
echo     # Clear status response
echo     while ser.in_waiting ^> 0:
echo         ser.read^(ser.in_waiting^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Device communication established'^)
echo     
echo     # Step 1: Create network
echo     print^('Creating network: %CHANNEL%'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(^('%network_cmd%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     
echo     network_response = ''
echo     while ser.in_waiting ^> 0:
echo         network_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Network response:', network_response[:200]^)
echo     
echo     # Step 2: Configure device
echo     print^('Configuring device parameters...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(^('%config_cmd%' + '\r\n'^).encode^(^)^)
echo     time.sleep^(4^)
echo     
echo     config_response = ''
echo     while ser.in_waiting ^> 0:
echo         config_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Configuration response:', config_response[:200]^)
echo     
echo     # Try alternative if needed
echo     if ^('comando desconocido' in config_response.lower^(^) or 
echo         'unknown command' in config_response.lower^(^) or
echo         len^(config_response.strip^(^)^) ^< 10^):
echo         
echo         print^('Trying alternative CONFIG command...'^)
echo         ser.reset_input_buffer^(^)
echo         ser.write^(^('%config_cmd_alt%' + '\r\n'^).encode^(^)^)
echo         time.sleep^(4^)
echo         
echo         while ser.in_waiting ^> 0:
echo             config_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo             time.sleep^(0.1^)
echo     
echo     # Step 3: Save configuration
echo     print^('Saving configuration to EEPROM...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'CONFIG_SAVE\r\n'^)
echo     time.sleep^(3^)
echo     
echo     save_response = ''
echo     while ser.in_waiting ^> 0:
echo         save_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     print^('Save response:', save_response[:100]^)
echo     
echo     # Step 4: Start device
echo     print^('Starting device operation...'^)
echo     ser.reset_input_buffer^(^)
echo     ser.write^(b'START\r\n'^)
echo     time.sleep^(2^)
echo     
echo     # Final status
echo     ser.write^(b'NETWORK_STATUS\r\n'^)
echo     time.sleep^(2^)
echo     
echo     final_response = ''
echo     while ser.in_waiting ^> 0:
echo         final_response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore'^)
echo         time.sleep^(0.1^)
echo     
echo     ser.close^(^)
echo     
echo     # Check for success
echo     all_responses = ^(network_response + config_response + save_response + final_response^).lower^(^)
echo     success_indicators = [
echo         'network creada exitosamente',
echo         'configuracion guardada exitosamente',
echo         'configuración guardada',
echo         'network activa',
echo         'listo y operando'
echo     ]
echo     
echo     if any^(indicator in all_responses for indicator in success_indicators^):
echo         print^('CONFIG_SUCCESS'^)
echo     else:
echo         print^('CONFIG_UNCERTAIN'^)
echo         
echo except Exception as e:
echo     print^('CONFIG_ERROR:', str^(e^)^)
) > "%SERIAL_SCRIPT%"

REM Execute configuration
python "%SERIAL_SCRIPT%" > "%TEMP%\config_result.txt" 2>&1
set "result="
for /f "tokens=*" %%i in ('type "%TEMP%\config_result.txt"') do (
    set "result=!result! %%i"
)

if "%result%"=="*CONFIG_SUCCESS*" (
    call :print_color %GREEN% "Configuration completed successfully"
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 0
) else if "%result%"=="*CONFIG_UNCERTAIN*" (
    call :print_color %YELLOW% "Configuration may have failed"
    call :print_manual_verification
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 0
) else (
    call :print_color %RED% "Configuration error"
    call :print_color %RED% "%result%"
    call :print_manual_config
    del "%SERIAL_SCRIPT%" 2>nul
    del "%TEMP%\config_result.txt" 2>nul
    exit /b 1
)
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the Python flash tool
REM Automatically opens in Command Prompt when executed

REM Global variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Colors (Windows doesn't support colors in batch, so we'll use text formatting)
set "RED=[ERROR]"
set "GREEN=[OK]"
set "YELLOW=[WARN]"
set "BLUE=[INFO]"

REM Function to print colored output (simulated with text)
:print_color
set "color=%1"
set "message=%2"
echo %color% %message%
goto :eof

:print_header
cls
echo ==================================
echo     Custodia Flash Tool v2.0     
echo        Windows Command Version    
echo ==================================
echo.
goto :eof

REM Function to check if command exists
:command_exists
where %1 >nul 2>&1
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to install Python if needed
:install_python
call :command_exists python
if %errorlevel% neq 0 (
    call :command_exists python3
    if %errorlevel% neq 0 (
        call :print_color %YELLOW% "Python not found. Please install Python manually:"
        call :print_color %YELLOW% "1. Download from https://python.org"
        call :print_color %YELLOW% "2. Make sure to check 'Add Python to PATH' during installation"
        call :print_color %YELLOW% "3. Restart Command Prompt after installation"
        exit /b 1
    )
)
exit /b 0

REM Function to install dependencies
:install_dependencies
call :print_color %YELLOW% "[SETUP] Checking dependencies..."

REM Install Python if needed
call :install_python
if %errorlevel% neq 0 (
    exit /b 1
)

REM Check pip
call :command_exists pip
if %errorlevel% neq 0 (
    call :command_exists pip3
    if %errorlevel% neq 0 (
        call :print_color %RED% "ERROR: pip not found"
        call :print_color %YELLOW% "Try: python -m ensurepip --upgrade"
        exit /b 1
    )
    set "PIP_CMD=pip3"
) else (
    set "PIP_CMD=pip"
)

REM Check/install pyserial
python -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Installing pyserial..."
    %PIP_CMD% install pyserial
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "pyserial installed successfully"
    ) else (
        call :print_color %RED% "Failed to install pyserial"
        exit /b 1
    )
) else (
    call :print_color %GREEN% "pyserial already installed"
)

exit /b 0

REM Function to find PlatformIO
:find_platformio
call :command_exists pio
if %errorlevel% equ 0 (
    pio --version >nul 2>&1
    if %errorlevel% equ 0 (
        set "PIO_CMD=pio"
        exit /b 0
    )
)

REM Check common installation paths
set "POSSIBLE_PATHS=%USERPROFILE%\.local\bin\pio.exe;%USERPROFILE%\.platformio\penv\Scripts\pio.exe;%USERPROFILE%\AppData\Local\Programs\Python\Scripts\pio.exe"

for %%p in (%POSSIBLE_PATHS%) do (
    if exist "%%p" (
        "%%p" --version >nul 2>&1
        if !errorlevel! equ 0 (
            set "PIO_CMD=%%p"
            exit /b 0
        )
    )
)

exit /b 1

REM Function to install PlatformIO
:install_platformio
call :print_color %YELLOW% "PlatformIO not found. Installing globally..."

if %PIP_CMD% install --user platformio (
    call :print_color %GREEN% "PlatformIO installed successfully (user)"
    
    REM Add to PATH for current session
    set "PATH=%PATH%;%USERPROFILE%\.local\bin;%USERPROFILE%\AppData\Roaming\Python\Scripts"
    
    timeout /t 3 /nobreak >nul
    
    REM Try to find it again
    call :find_platformio
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "PlatformIO ready: %PIO_CMD%"
        exit /b 0
    ) else (
        call :print_color %YELLOW% "Installation completed but PlatformIO not immediately available"
        call :print_color %YELLOW% "Please restart Command Prompt and try again"
        exit /b 1
    )
) else (
    call :print_color %RED% "Failed to install PlatformIO"
    call :print_color %YELLOW% "Manual installation:"
    call :print_color %YELLOW% "1. %PIP_CMD% install --user platformio"
    call :print_color %YELLOW% "2. Add %%USERPROFILE%%\.local\bin to your PATH"
    exit /b 1
)

REM Function to verify project structure
:verify_project_structure
call :print_color %BLUE% "Verifying project structure..."

if not exist "%PROJECT_DIR%" (
    call :print_color %RED% "ERROR: Project directory not found: %PROJECT_DIR%"
    exit /b 1
)

if not exist "%PROJECT_DIR%\platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in project directory"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\platformio.ini"
    exit /b 1
)

if not exist "%PROJECT_DIR%\src" (
    call :print_color %RED% "ERROR: src directory not found in project"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\src"
    exit /b 1
)

call :print_color %GREEN% "Project structure verified successfully"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Script directory: %SCRIPT_DIR%"
exit /b 0

REM Function to detect ESP32 devices
:detect_ports
call :print_color %YELLOW% "Detecting ESP32 devices..."

REM Create Python script for port detection
set "PORT_SCRIPT=%TEMP%\detect_ports.py"
(
echo import serial.tools.list_ports
echo import sys
echo.
echo ports = serial.tools.list_ports.comports^(^)
echo esp32_ports = []
echo.
echo for port in ports:
echo     device_lower = port.device.lower^(^)
echo     desc_lower = port.description.lower^(^)
echo     manufacturer_lower = ^(port.manufacturer or ""^).lower^(^)
echo     
echo     # Skip non-relevant devices
echo     skip_patterns = ["bluetooth", "debug-console", "hub", "keyboard", "mouse"]
echo     if any^(pattern in desc_lower for pattern in skip_patterns^):
echo         continue
echo     
echo     # ESP32 indicators
echo     esp32_indicators = ["cp210", "ch340", "ch341", "ft232", "pl2303", "esp32", "serial", "uart", "usb-serial", "com"]
echo     
echo     is_esp32_candidate = False
echo     
echo     if any^(pattern in device_lower for pattern in ["com"]^):
echo         is_esp32_candidate = True
echo     
echo     if any^(indicator in desc_lower for indicator in esp32_indicators^):
echo         is_esp32_candidate = True
echo     
echo     if any^(mfg in manufacturer_lower for mfg in ["silicon labs", "ftdi", "prolific", "ch340"]^):
echo         is_esp32_candidate = True
echo     
echo     if is_esp32_candidate:
echo         esp32_ports.append^((port.device, port.description^)^)
echo.
echo if not esp32_ports:
echo     print^("NO_ESP32_FOUND"^)
echo     if ports:
echo         print^("AVAILABLE_PORTS:"^)
echo         for i, port in enumerate^(ports, 1^):
echo             print^(f"{i}:{port.device}:{port.description}"^)
echo else:
echo     if len^(esp32_ports^) == 1:
echo         print^(f"SINGLE_ESP32:{esp32_ports[0][0]}:{esp32_ports[0][1]}"^)
echo     else:
echo         print^("MULTIPLE_ESP32:"^)
echo         for i, ^(device, desc^) in enumerate^(esp32_ports, 1^):
echo             print^(f"{i}:{device}:{desc}"^)
) > "%PORT_SCRIPT%"

python "%PORT_SCRIPT%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Could not detect ports (pyserial issue)"
    del "%PORT_SCRIPT%" 2>nul
    exit /b 1
)

REM Parse the output
for /f "tokens=*" %%i in ('python "%PORT_SCRIPT%"') do (
    set "PORT_INFO=%%i"
    if "!PORT_INFO!"=="NO_ESP32_FOUND" (
        call :print_color %YELLOW% "No ESP32 devices detected automatically."
        del "%PORT_SCRIPT%" 2>nul
        exit /b 1
    ) else if "!PORT_INFO:~0,13!"=="SINGLE_ESP32:" (
        for /f "tokens=2,3 delims=:" %%a in ("!PORT_INFO!") do (
            set "SELECTED_PORT=%%a"
            call :print_color %GREEN% "ESP32 device detected: %%a - %%b"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        )
    ) else if "!PORT_INFO!"=="MULTIPLE_ESP32:" (
        call :print_color %YELLOW% "Multiple ESP32 devices found:"
        set "PORT_COUNT=0"
        for /f "tokens=*" %%j in ('python "%PORT_SCRIPT%"') do (
            set "LINE=%%j"
            if "!LINE:~0,1!" neq "M" (
                set /a PORT_COUNT+=1
                for /f "tokens=1,2,3 delims=:" %%a in ("!LINE!") do (
                    echo   %%a. %%b - %%c
                    set "PORT_!PORT_COUNT!=%%b"
                )
            )
        )
        echo.
        :select_port
        set /p "choice=Select port [1-%PORT_COUNT%]: "
        if "!choice!" geq "1" if "!choice!" leq "%PORT_COUNT%" (
            call set "SELECTED_PORT=%%PORT_!choice!%%"
            call :print_color %GREEN% "Selected: !SELECTED_PORT!"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        ) else (
            call :print_color %RED% "Invalid selection. Please enter a number between 1 and %PORT_COUNT%"
            goto select_port
        )
    )
)

del "%PORT_SCRIPT%" 2>nul
exit /b 1

REM Function to detect board type (only XIAO ESP32S3 now)
:detect_board_type
set "BOARD_TYPE=seeed_xiao_esp32s3"
call :print_color %BLUE% "Using board: XIAO ESP32S3"
exit /b 0

REM Function to validate role
:validate_role
set "role=%1"
set "role_upper=%role%"
call :to_upper role_upper

if "%role_upper%"=="TRACKER" exit /b 0
if "%role_upper%"=="RECEIVER" exit /b 0
if "%role_upper%"=="REPEATER" exit /b 0
exit /b 1

REM Function to get role with validation
:get_role
:role_loop
echo.
call :print_color %BLUE% "Device Role:"
echo   1. TRACKER  - GPS tracking device
echo   2. RECEIVER - Base station/receiver
echo   3. REPEATER - Signal repeater/relay
echo.
set /p "choice=Select role [1-3]: "

if "%choice%"=="1" (
    set "ROLE=TRACKER"
    goto :eof
) else if "%choice%"=="2" (
    set "ROLE=RECEIVER"
    goto :eof
) else if "%choice%"=="3" (
    set "ROLE=REPEATER"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto role_loop
)

REM Function to validate device ID
:validate_device_id
set "id=%1"
if %id% geq 1 if %id% leq 255 exit /b 0
exit /b 1

REM Function to get device ID with validation
:get_device_id
:id_loop
echo.
call :print_color %BLUE% "Device ID:"
echo   Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

call :validate_device_id %DEVICE_ID%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid device ID. Please enter a number between 1 and 255"
    goto id_loop
)

REM Function to validate interval
:validate_interval
set "interval=%1"
if %interval% geq 5 if %interval% leq 3600 exit /b 0
exit /b 1

REM Function to get interval with validation
:get_interval
:interval_loop
echo.
call :print_color %BLUE% "Transmission Interval:"
echo   How often device sends GPS updates (in seconds)
echo   Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

call :validate_interval %INTERVAL%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid interval. Please enter a number between 5 and 3600 seconds"
    goto interval_loop
)

REM Function to get region with validation
:get_region
:region_loop
echo.
call :print_color %BLUE% "LoRa Region:"
echo   1. US - United States
echo   2. EU - Europe
echo   3. CH - China
echo   4. AS - Asia
echo   5. JP - Japan
echo.
set /p "choice=Select region [1-5]: "

if "%choice%"=="1" (
    set "REGION=US"
    goto :eof
) else if "%choice%"=="2" (
    set "REGION=EU"
    goto :eof
) else if "%choice%"=="3" (
    set "REGION=CH"
    goto :eof
) else if "%choice%"=="4" (
    set "REGION=AS"
    goto :eof
) else if "%choice%"=="5" (
    set "REGION=JP"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, 3, 4, or 5"
    goto region_loop
)

REM Function to get mode with validation
:get_mode
:mode_loop
echo.
call :print_color %BLUE% "Operation Mode:"
echo   1. SIMPLE - Basic operation
echo   2. ADMIN  - Advanced features enabled
echo.
set /p "choice=Select mode [1-2]: "

if "%choice%"=="1" (
    set "MODE=SIMPLE"
    goto :eof
) else if "%choice%"=="2" (
    set "MODE=ADMIN"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1 or 2"
    goto mode_loop
)

REM Function to get custom advanced parameters
:get_custom_advanced_params
echo.
call :print_color %YELLOW% "Custom Advanced Radio Parameters:"
echo.

REM Bandwidth
:bw_loop
call :print_color %BLUE% "Bandwidth (kHz):"
echo   1. 125 kHz (default)
echo   2. 250 kHz
echo   3. 500 kHz
echo.
set /p "choice=Select bandwidth [1-3]: "

if "%choice%"=="1" (
    set "CUSTOM_BW=125"
    goto sf_loop
) else if "%choice%"=="2" (
    set "CUSTOM_BW=250"
    goto sf_loop
) else if "%choice%"=="3" (
    set "CUSTOM_BW=500"
    goto sf_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto bw_loop
)

REM Spreading Factor
:sf_loop
call :print_color %BLUE% "Spreading Factor:"
echo   1. SF7  (fastest, shortest range)
echo   2. SF8
echo   3. SF9
echo   4. SF10
echo   5. SF11
echo   6. SF12 (slowest, longest range)
echo.
set /p "choice=Select spreading factor [1-6]: "

if "%choice%"=="1" (
    set "CUSTOM_SF=7"
    goto cr_loop
) else if "%choice%"=="2" (
    set "CUSTOM_SF=8"
    goto cr_loop
) else if "%choice%"=="3" (
    set "CUSTOM_SF=9"
    goto cr_loop
) else if "%choice%"=="4" (
    set "CUSTOM_SF=10"
    goto cr_loop
) else if "%choice%"=="5" (
    set "CUSTOM_SF=11"
    goto cr_loop
) else if "%choice%"=="6" (
    set "CUSTOM_SF=12"
    goto cr_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-6"
    goto sf_loop
)

REM Coding Rate
:cr_loop
call :print_color %BLUE% "Coding Rate:"
echo   1. 4/5 (fastest)
echo   2. 4/6
echo   3. 4/7
echo   4. 4/8 (most robust)
echo.
set /p "choice=Select coding rate [1-4]: "

if "%choice%"=="1" (
    set "CUSTOM_CR=5"
    goto power_loop
) else if "%choice%"=="2" (
    set "CUSTOM_CR=6"
    goto power_loop
) else if "%choice%"=="3" (
    set "CUSTOM_CR=7"
    goto power_loop
) else if "%choice%"=="4" (
    set "CUSTOM_CR=8"
    goto power_loop
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-4"
    goto cr_loop
)

REM TX Power
:power_loop
call :print_color %BLUE% "TX Power (dBm):"
echo   Range: 2-22 dBm
echo   Recommended: 14-20 dBm
echo.
set /p "CUSTOM_POWER=Enter TX power [2-22]: "

if %CUSTOM_POWER% geq 2 if %CUSTOM_POWER% leq 22 (
    goto preamble_loop
) else (
    call :print_color %RED% "Invalid power. Please enter a number between 2 and 22"
    goto power_loop
)

REM Preamble Length
:preamble_loop
call :print_color %BLUE% "Preamble Length:"
echo   Range: 6-65535
echo   Default: 8
echo.
set /p "CUSTOM_PREAMBLE=Enter preamble length [6-65535]: "

if %CUSTOM_PREAMBLE% geq 6 if %CUSTOM_PREAMBLE% leq 65535 (
    goto :eof
) else (
    call :print_color %RED% "Invalid preamble length. Please enter a number between 6 and 65535"
    goto preamble_loop
)

REM Function to get radio profile with validation
:get_radio
:radio_loop
echo.
call :print_color %BLUE% "Radio Profile:"
echo   1. DESERT_LONG_FAST  - Long range, fast transmission
echo   2. MOUNTAIN_STABLE   - Stable connection in mountains
echo   3. URBAN_DENSE       - Dense urban environments
echo   4. MESH_MAX_NODES    - Maximum mesh network nodes
echo   5. CUSTOM_ADVANCED   - Custom advanced settings
echo.
set /p "choice=Select radio profile [1-5]: "

if "%choice%"=="1" (
    set "RADIO=DESERT_LONG_FAST"
    goto :eof
) else if "%choice%"=="2" (
    set "RADIO=MOUNTAIN_STABLE"
    goto :eof
) else if "%choice%"=="3" (
    set "RADIO=URBAN_DENSE"
    goto :eof
) else if "%choice%"=="4" (
    set "RADIO=MESH_MAX_NODES"
    goto :eof
) else if "%choice%"=="5" (
    set "RADIO=CUSTOM_ADVANCED"
    call :get_custom_advanced_params
    call :print_color %YELLOW% "Note: Custom advanced parameters will be configured in the main tool"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1-5"
    goto radio_loop
)

REM Function to validate hops
:validate_hops
set "hops=%1"
if %hops% geq 1 if %hops% leq 10 exit /b 0
exit /b 1

REM Function to get hops with validation
:get_hops
:hops_loop
echo.
call :print_color %BLUE% "Max Hops:"
echo   Maximum number of hops for mesh network (1-10)
echo   Default: 3
echo.
set /p "input=Enter max hops [1-10] or press Enter for default (3): "

if "%input%"=="" (
    set "HOPS=3"
    goto :eof
) else (
    call :validate_hops %input%
    if %errorlevel% equ 0 (
        set "HOPS=%input%"
        goto :eof
    ) else (
        call :print_color %RED% "Invalid hops. Please enter a number between 1 and 10"
        goto hops_loop
    )
)

REM Function to validate channel name
:validate_channel
set "channel=%1"
call :to_upper channel

REM Check length (simplified for batch)
if "%channel:~3%"=="" exit /b 1
if "%channel:~20%" neq "" exit /b 1

REM Check for reserved names
if "%channel%"=="CONFIG" exit /b 1
if "%channel%"=="ADMIN" exit /b 1
if "%channel%"=="DEBUG" exit /b 1
if "%channel%"=="SYSTEM" exit /b 1
if "%channel%"=="DEVICE" exit /b 1
if "%channel%"=="LORA" exit /b 1
if "%channel%"=="MESH" exit /b 1
if "%channel%"=="TEST" exit /b 1
if "%channel%"=="DEFAULT" exit /b 1

exit /b 0

REM Function to get channel with validation
:get_channel
:channel_loop
echo.
call :print_color %BLUE% "Network Channel:"
echo   Network name for your devices (3-20 characters)
echo   Only letters, numbers, and underscore allowed
echo   Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS
echo.
set /p "CHANNEL=Enter channel name: "

call :validate_channel "%CHANNEL%"
if %errorlevel% equ 0 (
    call :to_upper CHANNEL
    goto :eof
) else (
    call :print_color %RED% "Invalid channel name. Requirements:"
    call :print_color %RED% "  - 3-20 characters"
    call :print_color %RED% "  - Only letters, numbers, underscore"
    call :print_color %RED% "  - Cannot be: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT"
    goto channel_loop
)

REM Function to validate password
:validate_password
set "password=%1"
call :to_upper password

REM Check length (simplified for batch)
if "%password:~8%"=="" exit /b 1
if "%password:~32%" neq "" exit /b 1

REM Check if same as channel
if "%password%"=="%CHANNEL%" exit /b 1

exit /b 0

REM Function to generate password
:generate_password
set "chars=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
set "PASSWORD="

REM Generate 8-character password (simplified)
for /l %%i in (1,1,8) do (
    set /a "rand=!random! %% 36"
    call set "char=!chars:~!rand!,1!"
    set "PASSWORD=!PASSWORD!!char!"
)
goto :eof

REM Function to get password with validation
:get_password
:password_loop
echo.
call :print_color %BLUE% "Network Password:"
echo   Password requirements:
echo   - 8-32 characters
echo   - Must contain at least 1 number and 1 letter
echo   - Cannot be the same as channel name
echo   Examples: Secure123, MyPass99, Field2024
echo.
set /p "input=Enter password or press Enter for auto-generation: "

if "%input%"=="" (
    call :generate_password
    call :print_color %GREEN% "Password auto-generated: %PASSWORD%"
    goto :eof
) else (
    call :validate_password "%input%"
    if %errorlevel% equ 0 (
        set "PASSWORD=%input%"
        call :to_upper PASSWORD
        goto :eof
    ) else (
        call :print_color %RED% "Invalid password. Requirements:"
        call :print_color %RED% "  - 8-32 characters"
        call :print_color %RED% "  - At least 1 number and 1 letter"
        call :print_color %RED% "  - Cannot be same as channel name"
        goto password_loop
    )
)

REM Function to convert to uppercase
:to_upper
set "str=%1"
set "result="
for /f "delims=" %%i in ('cmd /c "echo %str%"') do (
    set "result=%%i"
)
set "%1=%result%"
goto :eof
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the Python flash tool
REM Automatically opens in Command Prompt when executed

REM Global variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Colors (Windows doesn't support colors in batch, so we'll use text formatting)
set "RED=[ERROR]"
set "GREEN=[OK]"
set "YELLOW=[WARN]"
set "BLUE=[INFO]"

REM Function to print colored output (simulated with text)
:print_color
set "color=%1"
set "message=%2"
echo %color% %message%
goto :eof

:print_header
cls
echo ==================================
echo     Custodia Flash Tool v2.0     
echo        Windows Command Version    
echo ==================================
echo.
goto :eof

REM Function to check if command exists
:command_exists
where %1 >nul 2>&1
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to install Python if needed
:install_python
call :command_exists python
if %errorlevel% neq 0 (
    call :command_exists python3
    if %errorlevel% neq 0 (
        call :print_color %YELLOW% "Python not found. Please install Python manually:"
        call :print_color %YELLOW% "1. Download from https://python.org"
        call :print_color %YELLOW% "2. Make sure to check 'Add Python to PATH' during installation"
        call :print_color %YELLOW% "3. Restart Command Prompt after installation"
        exit /b 1
    )
)
exit /b 0

REM Function to install dependencies
:install_dependencies
call :print_color %YELLOW% "[SETUP] Checking dependencies..."

REM Install Python if needed
call :install_python
if %errorlevel% neq 0 (
    exit /b 1
)

REM Check pip
call :command_exists pip
if %errorlevel% neq 0 (
    call :command_exists pip3
    if %errorlevel% neq 0 (
        call :print_color %RED% "ERROR: pip not found"
        call :print_color %YELLOW% "Try: python -m ensurepip --upgrade"
        exit /b 1
    )
    set "PIP_CMD=pip3"
) else (
    set "PIP_CMD=pip"
)

REM Check/install pyserial
python -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Installing pyserial..."
    %PIP_CMD% install pyserial
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "pyserial installed successfully"
    ) else (
        call :print_color %RED% "Failed to install pyserial"
        exit /b 1
    )
) else (
    call :print_color %GREEN% "pyserial already installed"
)

exit /b 0

REM Function to find PlatformIO
:find_platformio
call :command_exists pio
if %errorlevel% equ 0 (
    pio --version >nul 2>&1
    if %errorlevel% equ 0 (
        set "PIO_CMD=pio"
        exit /b 0
    )
)

REM Check common installation paths
set "POSSIBLE_PATHS=%USERPROFILE%\.local\bin\pio.exe;%USERPROFILE%\.platformio\penv\Scripts\pio.exe;%USERPROFILE%\AppData\Local\Programs\Python\Scripts\pio.exe"

for %%p in (%POSSIBLE_PATHS%) do (
    if exist "%%p" (
        "%%p" --version >nul 2>&1
        if !errorlevel! equ 0 (
            set "PIO_CMD=%%p"
            exit /b 0
        )
    )
)

exit /b 1

REM Function to install PlatformIO
:install_platformio
call :print_color %YELLOW% "PlatformIO not found. Installing globally..."

if %PIP_CMD% install --user platformio (
    call :print_color %GREEN% "PlatformIO installed successfully (user)"
    
    REM Add to PATH for current session
    set "PATH=%PATH%;%USERPROFILE%\.local\bin;%USERPROFILE%\AppData\Roaming\Python\Scripts"
    
    timeout /t 3 /nobreak >nul
    
    REM Try to find it again
    call :find_platformio
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "PlatformIO ready: %PIO_CMD%"
        exit /b 0
    ) else (
        call :print_color %YELLOW% "Installation completed but PlatformIO not immediately available"
        call :print_color %YELLOW% "Please restart Command Prompt and try again"
        exit /b 1
    )
) else (
    call :print_color %RED% "Failed to install PlatformIO"
    call :print_color %YELLOW% "Manual installation:"
    call :print_color %YELLOW% "1. %PIP_CMD% install --user platformio"
    call :print_color %YELLOW% "2. Add %%USERPROFILE%%\.local\bin to your PATH"
    exit /b 1
)

REM Function to verify project structure
:verify_project_structure
call :print_color %BLUE% "Verifying project structure..."

if not exist "%PROJECT_DIR%" (
    call :print_color %RED% "ERROR: Project directory not found: %PROJECT_DIR%"
    exit /b 1
)

if not exist "%PROJECT_DIR%\platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in project directory"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\platformio.ini"
    exit /b 1
)

if not exist "%PROJECT_DIR%\src" (
    call :print_color %RED% "ERROR: src directory not found in project"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\src"
    exit /b 1
)

call :print_color %GREEN% "Project structure verified successfully"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Script directory: %SCRIPT_DIR%"
exit /b 0

REM Function to detect ESP32 devices
:detect_ports
call :print_color %YELLOW% "Detecting ESP32 devices..."

REM Create Python script for port detection
set "PORT_SCRIPT=%TEMP%\detect_ports.py"
(
echo import serial.tools.list_ports
echo import sys
echo.
echo ports = serial.tools.list_ports.comports^(^)
echo esp32_ports = []
echo.
echo for port in ports:
echo     device_lower = port.device.lower^(^)
echo     desc_lower = port.description.lower^(^)
echo     manufacturer_lower = ^(port.manufacturer or ""^).lower^(^)
echo     
echo     # Skip non-relevant devices
echo     skip_patterns = ["bluetooth", "debug-console", "hub", "keyboard", "mouse"]
echo     if any^(pattern in desc_lower for pattern in skip_patterns^):
echo         continue
echo     
echo     # ESP32 indicators
echo     esp32_indicators = ["cp210", "ch340", "ch341", "ft232", "pl2303", "esp32", "serial", "uart", "usb-serial", "com"]
echo     
echo     is_esp32_candidate = False
echo     
echo     if any^(pattern in device_lower for pattern in ["com"]^):
echo         is_esp32_candidate = True
echo     
echo     if any^(indicator in desc_lower for indicator in esp32_indicators^):
echo         is_esp32_candidate = True
echo     
echo     if any^(mfg in manufacturer_lower for mfg in ["silicon labs", "ftdi", "prolific", "ch340"]^):
echo         is_esp32_candidate = True
echo     
echo     if is_esp32_candidate:
echo         esp32_ports.append^((port.device, port.description^)^)
echo.
echo if not esp32_ports:
echo     print^("NO_ESP32_FOUND"^)
echo     if ports:
echo         print^("AVAILABLE_PORTS:"^)
echo         for i, port in enumerate^(ports, 1^):
echo             print^(f"{i}:{port.device}:{port.description}"^)
echo else:
echo     if len^(esp32_ports^) == 1:
echo         print^(f"SINGLE_ESP32:{esp32_ports[0][0]}:{esp32_ports[0][1]}"^)
echo     else:
echo         print^("MULTIPLE_ESP32:"^)
echo         for i, ^(device, desc^) in enumerate^(esp32_ports, 1^):
echo             print^(f"{i}:{device}:{desc}"^)
) > "%PORT_SCRIPT%"

python "%PORT_SCRIPT%"
if %errorlevel% neq 0 (
    call :print_color %RED% "ERROR: Could not detect ports (pyserial issue)"
    del "%PORT_SCRIPT%" 2>nul
    exit /b 1
)

REM Parse the output
for /f "tokens=*" %%i in ('python "%PORT_SCRIPT%"') do (
    set "PORT_INFO=%%i"
    if "!PORT_INFO!"=="NO_ESP32_FOUND" (
        call :print_color %YELLOW% "No ESP32 devices detected automatically."
        del "%PORT_SCRIPT%" 2>nul
        exit /b 1
    ) else if "!PORT_INFO:~0,13!"=="SINGLE_ESP32:" (
        for /f "tokens=2,3 delims=:" %%a in ("!PORT_INFO!") do (
            set "SELECTED_PORT=%%a"
            call :print_color %GREEN% "ESP32 device detected: %%a - %%b"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        )
    ) else if "!PORT_INFO!"=="MULTIPLE_ESP32:" (
        call :print_color %YELLOW% "Multiple ESP32 devices found:"
        set "PORT_COUNT=0"
        for /f "tokens=*" %%j in ('python "%PORT_SCRIPT%"') do (
            set "LINE=%%j"
            if "!LINE:~0,1!" neq "M" (
                set /a PORT_COUNT+=1
                for /f "tokens=1,2,3 delims=:" %%a in ("!LINE!") do (
                    echo   %%a. %%b - %%c
                    set "PORT_!PORT_COUNT!=%%b"
                )
            )
        )
        echo.
        :select_port
        set /p "choice=Select port [1-%PORT_COUNT%]: "
        if "!choice!" geq "1" if "!choice!" leq "%PORT_COUNT%" (
            call set "SELECTED_PORT=%%PORT_!choice!%%"
            call :print_color %GREEN% "Selected: !SELECTED_PORT!"
            del "%PORT_SCRIPT%" 2>nul
            exit /b 0
        ) else (
            call :print_color %RED% "Invalid selection. Please enter a number between 1 and %PORT_COUNT%"
            goto select_port
        )
    )
)

del "%PORT_SCRIPT%" 2>nul
exit /b 1

REM Function to detect board type (only XIAO ESP32S3 now)
:detect_board_type
set "BOARD_TYPE=seeed_xiao_esp32s3"
call :print_color %BLUE% "Using board: XIAO ESP32S3"
exit /b 0

REM Function to validate role
:validate_role
set "role=%1"
set "role_upper=%role%"
call :to_upper role_upper

if "%role_upper%"=="TRACKER" exit /b 0
if "%role_upper%"=="RECEIVER" exit /b 0
if "%role_upper%"=="REPEATER" exit /b 0
exit /b 1

REM Function to get role with validation
:get_role
:role_loop
echo.
call :print_color %BLUE% "Device Role:"
echo   1. TRACKER  - GPS tracking device
echo   2. RECEIVER - Base station/receiver
echo   3. REPEATER - Signal repeater/relay
echo.
set /p "choice=Select role [1-3]: "

if "%choice%"=="1" (
    set "ROLE=TRACKER"
    goto :eof
) else if "%choice%"=="2" (
    set "ROLE=RECEIVER"
    goto :eof
) else if "%choice%"=="3" (
    set "ROLE=REPEATER"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, or 3"
    goto role_loop
)

REM Function to validate device ID
:validate_device_id
set "id=%1"
if %id% geq 1 if %id% leq 255 exit /b 0
exit /b 1

REM Function to get device ID with validation
:get_device_id
:id_loop
echo.
call :print_color %BLUE% "Device ID:"
echo   Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

call :validate_device_id %DEVICE_ID%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid device ID. Please enter a number between 1 and 255"
    goto id_loop
)

REM Function to validate interval
:validate_interval
set "interval=%1"
if %interval% geq 5 if %interval% leq 3600 exit /b 0
exit /b 1

REM Function to get interval with validation
:get_interval
:interval_loop
echo.
call :print_color %BLUE% "Transmission Interval:"
echo   How often device sends GPS updates (in seconds)
echo   Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

call :validate_interval %INTERVAL%
if %errorlevel% equ 0 (
    goto :eof
) else (
    call :print_color %RED% "Invalid interval. Please enter a number between 5 and 3600 seconds"
    goto interval_loop
)

REM Function to get region with validation
:get_region
:region_loop
echo.
call :print_color %BLUE% "LoRa Region:"
echo   1. US - United States
echo   2. EU - Europe
echo   3. CH - China
echo   4. AS - Asia
echo   5. JP - Japan
echo.
set /p "choice=Select region [1-5]: "

if "%choice%"=="1" (
    set "REGION=US"
    goto :eof
) else if "%choice%"=="2" (
    set "REGION=EU"
    goto :eof
) else if "%choice%"=="3" (
    set "REGION=CH"
    goto :eof
) else if "%choice%"=="4" (
    set "REGION=AS"
    goto :eof
) else if "%choice%"=="5" (
    set "REGION=JP"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1, 2, 3, 4, or 5"
    goto region_loop
)

REM Function to get mode with validation
:get_mode
:mode_loop
echo.
call :print_color %BLUE% "Operation Mode:"
echo   1. SIMPLE - Basic operation
echo   2. ADMIN  - Advanced features enabled
echo.
set /p "choice=Select mode [1-2]: "

if "%choice%"=="1" (
    set "MODE=SIMPLE"
    goto :eof
) else if "%choice%"=="2" (
    set "MODE=ADMIN"
    goto :eof
) else (
    call :print_color %RED% "Invalid selection. Please enter 1 or 2"
    goto mode_loop
)
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the Python flash tool
REM Automatically opens in Command Prompt when executed

REM Global variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Colors (Windows doesn't support colors in batch, so we'll use text formatting)
set "RED=[ERROR]"
set "GREEN=[OK]"
set "YELLOW=[WARN]"
set "BLUE=[INFO]"

REM Function to print colored output (simulated with text)
:print_color
set "color=%1"
set "message=%2"
echo %color% %message%
goto :eof

:print_header
cls
echo ==================================
echo     Custodia Flash Tool v2.0     
echo        Windows Command Version    
echo ==================================
echo.
goto :eof

REM Function to check if command exists
:command_exists
where %1 >nul 2>&1
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to install Python if needed
:install_python
call :command_exists python
if %errorlevel% neq 0 (
    call :command_exists python3
    if %errorlevel% neq 0 (
        call :print_color %YELLOW% "Python not found. Please install Python manually:"
        call :print_color %YELLOW% "1. Download from https://python.org"
        call :print_color %YELLOW% "2. Make sure to check 'Add Python to PATH' during installation"
        call :print_color %YELLOW% "3. Restart Command Prompt after installation"
        exit /b 1
    )
)
exit /b 0

REM Function to install dependencies
:install_dependencies
call :print_color %YELLOW% "[SETUP] Checking dependencies..."

REM Install Python if needed
call :install_python
if %errorlevel% neq 0 (
    exit /b 1
)

REM Check pip
call :command_exists pip
if %errorlevel% neq 0 (
    call :command_exists pip3
    if %errorlevel% neq 0 (
        call :print_color %RED% "ERROR: pip not found"
        call :print_color %YELLOW% "Try: python -m ensurepip --upgrade"
        exit /b 1
    )
    set "PIP_CMD=pip3"
) else (
    set "PIP_CMD=pip"
)

REM Check/install pyserial
python -c "import serial" >nul 2>&1
if %errorlevel% neq 0 (
    call :print_color %YELLOW% "Installing pyserial..."
    %PIP_CMD% install pyserial
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "pyserial installed successfully"
    ) else (
        call :print_color %RED% "Failed to install pyserial"
        exit /b 1
    )
) else (
    call :print_color %GREEN% "pyserial already installed"
)

exit /b 0

REM Function to find PlatformIO
:find_platformio
call :command_exists pio
if %errorlevel% equ 0 (
    pio --version >nul 2>&1
    if %errorlevel% equ 0 (
        set "PIO_CMD=pio"
        exit /b 0
    )
)

REM Check common installation paths
set "POSSIBLE_PATHS=%USERPROFILE%\.local\bin\pio.exe;%USERPROFILE%\.platformio\penv\Scripts\pio.exe;%USERPROFILE%\AppData\Local\Programs\Python\Scripts\pio.exe"

for %%p in (%POSSIBLE_PATHS%) do (
    if exist "%%p" (
        "%%p" --version >nul 2>&1
        if !errorlevel! equ 0 (
            set "PIO_CMD=%%p"
            exit /b 0
        )
    )
)

exit /b 1

REM Function to install PlatformIO
:install_platformio
call :print_color %YELLOW% "PlatformIO not found. Installing globally..."

if %PIP_CMD% install --user platformio (
    call :print_color %GREEN% "PlatformIO installed successfully (user)"
    
    REM Add to PATH for current session
    set "PATH=%PATH%;%USERPROFILE%\.local\bin;%USERPROFILE%\AppData\Roaming\Python\Scripts"
    
    timeout /t 3 /nobreak >nul
    
    REM Try to find it again
    call :find_platformio
    if %errorlevel% equ 0 (
        call :print_color %GREEN% "PlatformIO ready: %PIO_CMD%"
        exit /b 0
    ) else (
        call :print_color %YELLOW% "Installation completed but PlatformIO not immediately available"
        call :print_color %YELLOW% "Please restart Command Prompt and try again"
        exit /b 1
    )
) else (
    call :print_color %RED% "Failed to install PlatformIO"
    call :print_color %YELLOW% "Manual installation:"
    call :print_color %YELLOW% "1. %PIP_CMD% install --user platformio"
    call :print_color %YELLOW% "2. Add %%USERPROFILE%%\.local\bin to your PATH"
    exit /b 1
)

REM Function to verify project structure
:verify_project_structure
call :print_color %BLUE% "Verifying project structure..."

if not exist "%PROJECT_DIR%" (
    call :print_color %RED% "ERROR: Project directory not found: %PROJECT_DIR%"
    exit /b 1
)

if not exist "%PROJECT_DIR%\platformio.ini" (
    call :print_color %RED% "ERROR: platformio.ini not found in project directory"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\platformio.ini"
    exit /b 1
)

if not exist "%PROJECT_DIR%\src" (
    call :print_color %RED% "ERROR: src directory not found in project"
    call :print_color %YELLOW% "Expected location: %PROJECT_DIR%\src"
    exit /b 1
)

call :print_color %GREEN% "Project structure verified successfully"
call :print_color %BLUE% "Project directory: %PROJECT_DIR%"
call :print_color %BLUE% "Script directory: %SCRIPT_DIR%"
exit /b 0
