@echo off
setlocal enabledelayedexpansion

REM Custodia Flash Tool for Windows
REM Batch script version of the macOS flash tool
REM Automatically flashes and configures ESP32 devices

REM Global variables
set "SCRIPT_DIR=%~dp0"
REM Remove trailing backslash if present
if "!SCRIPT_DIR:~-1!"=="\" set "SCRIPT_DIR=!SCRIPT_DIR:~0,-1!"
set "PROJECT_DIR=!SCRIPT_DIR!"
set "PIO_CMD="
set "SELECTED_PORT="
set "BOARD_TYPE=seeed_xiao_esp32s3"

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

REM Custom advanced radio parameters
set "CUSTOM_BW="
set "CUSTOM_SF="
set "CUSTOM_CR="
set "CUSTOM_POWER="
set "CUSTOM_PREAMBLE="

REM Print header
call :print_header

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

:print_header
cls
echo.
echo ==================================
echo     Custodia Flash Tool v2.0     
echo     Windows Batch Version    
echo ==================================
echo.
goto :eof

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
echo This launcher will help you configure and flash your ESP32 device.
echo.

REM Configuration type selection
:config_type_loop
echo Configuration Type:
echo    1. Normal     - Interactive step-by-step configuration
echo    2. One Line   - Single command line configuration
echo.
set /p "config_choice=Select configuration type [1-2]: "

if "!config_choice!"=="1" (
    set "CONFIG_TYPE=normal"
    goto :normal_config
)
if "!config_choice!"=="2" (
    set "CONFIG_TYPE=oneline"
    goto :oneline_config
)
echo Invalid selection. Please enter 1 or 2
goto :config_type_loop

:oneline_config
echo One-line Configuration Mode
echo.
echo Simply enter the parameters - the script will process them automatically!
echo.
echo Example:
echo -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3
echo.
echo Available parameters:
echo   -role [TRACKER^|RECEIVER^|REPEATER]
echo   -id [1-255]
echo   -interval [5-3600]
echo   -region [US^|EU^|CH^|AS^|JP]
echo   -mode [SIMPLE^|ADMIN]
echo   -radio [DESERT_LONG_FAST^|MOUNTAIN_STABLE^|URBAN_DENSE^|MESH_MAX_NODES^|CUSTOM_ADVANCED]
echo   -channel [channel_name]
echo   -password [password] (optional)
echo   -hops [1-10] (optional, default: 3)
echo.

set /p "config_params=Enter your configuration parameters: "

REM Execute with parameters
call "%~f0" !config_params!
goto :end

:normal_config
echo Please provide the following configuration parameters:
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
REM Device ID
:device_id_loop
echo.
echo 2. Device ID:
echo    Unique identifier for this device (1-255)
echo.
set /p "DEVICE_ID=Enter device ID [1-255]: "

REM Simple validation
if !DEVICE_ID! lss 1 goto :device_id_error
if !DEVICE_ID! gtr 255 goto :device_id_error
goto :interval

:device_id_error
echo Invalid device ID. Please enter a number between 1 and 255
goto :device_id_loop

:interval
REM Transmission Interval
:interval_loop
echo.
echo 3. Transmission Interval:
echo    How often device sends GPS updates (in seconds)
echo    Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)
echo.
set /p "INTERVAL=Enter interval in seconds [5-3600]: "

REM Simple validation
if !INTERVAL! lss 5 goto :interval_error
if !INTERVAL! gtr 3600 goto :interval_error
goto :region

:interval_error
echo Invalid interval. Please enter a number between 5 and 3600 seconds
goto :interval_loop

:region
REM Region
:region_loop
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
goto :region_loop

:mode
REM Mode
:mode_loop
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
goto :mode_loop

:radio
REM Radio Profile
:radio_loop
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
    call :get_custom_params
    goto :hops
)
echo Invalid selection. Please enter 1-5
goto :radio_loop

:get_custom_params
echo.
echo Custom Advanced Parameters:
echo.

REM Bandwidth
:bandwidth_loop
echo Bandwidth (kHz):
echo   1. 7.8 kHz     2. 10.4 kHz    3. 15.6 kHz
echo   4. 20.8 kHz    5. 31.25 kHz   6. 41.7 kHz
echo   7. 62.5 kHz    8. 125 kHz     9. 250 kHz    10. 500 kHz
echo.
set /p "choice=Select bandwidth [1-10]: "

if "!choice!"=="1" set "CUSTOM_BW=7800"
if "!choice!"=="2" set "CUSTOM_BW=10400"
if "!choice!"=="3" set "CUSTOM_BW=15600"
if "!choice!"=="4" set "CUSTOM_BW=20800"
if "!choice!"=="5" set "CUSTOM_BW=31250"
if "!choice!"=="6" set "CUSTOM_BW=41700"
if "!choice!"=="7" set "CUSTOM_BW=62500"
if "!choice!"=="8" set "CUSTOM_BW=125000"
if "!choice!"=="9" set "CUSTOM_BW=250000"
if "!choice!"=="10" set "CUSTOM_BW=500000"

if "!CUSTOM_BW!"=="" (
    echo Invalid selection. Please enter 1-10
    goto :bandwidth_loop
)

REM Spreading Factor
:sf_loop
echo.
echo Spreading Factor:
echo   1. 6 (fastest, shortest range)    2. 7    3. 8    4. 9
echo   5. 10    6. 11    7. 12 (slowest, longest range)
echo.
set /p "choice=Select spreading factor [1-7]: "

if "!choice!"=="1" set "CUSTOM_SF=6"
if "!choice!"=="2" set "CUSTOM_SF=7"
if "!choice!"=="3" set "CUSTOM_SF=8"
if "!choice!"=="4" set "CUSTOM_SF=9"
if "!choice!"=="5" set "CUSTOM_SF=10"
if "!choice!"=="6" set "CUSTOM_SF=11"
if "!choice!"=="7" set "CUSTOM_SF=12"

if "!CUSTOM_SF!"=="" (
    echo Invalid selection. Please enter 1-7
    goto :sf_loop
)

REM Coding Rate
:cr_loop
echo.
echo Coding Rate:
echo   1. 4/5 (fastest)    2. 4/6    3. 4/7    4. 4/8 (most robust)
echo.
set /p "choice=Select coding rate [1-4]: "

if "!choice!"=="1" set "CUSTOM_CR=5"
if "!choice!"=="2" set "CUSTOM_CR=6"
if "!choice!"=="3" set "CUSTOM_CR=7"
if "!choice!"=="4" set "CUSTOM_CR=8"

if "!CUSTOM_CR!"=="" (
    echo Invalid selection. Please enter 1-4
    goto :cr_loop
)

REM TX Power
:power_loop
echo.
echo TX Power (dBm):
echo   Range: 2-22 dBm
echo   Recommended: 14-20 dBm
echo.
set /p "CUSTOM_POWER=Enter TX power [2-22]: "

if !CUSTOM_POWER! lss 2 goto :power_error
if !CUSTOM_POWER! gtr 22 goto :power_error
goto :preamble

:power_error
echo Invalid power. Please enter a number between 2 and 22
goto :power_loop

:preamble
REM Preamble Length
:preamble_loop
echo.
echo Preamble Length:
echo   Range: 6-65535
echo   Default: 8
echo.
set /p "CUSTOM_PREAMBLE=Enter preamble length [6-65535]: "

if !CUSTOM_PREAMBLE! lss 6 goto :preamble_error
if !CUSTOM_PREAMBLE! gtr 65535 goto :preamble_error
goto :eof

:preamble_error
echo Invalid preamble length. Please enter a number between 6 and 65535
goto :preamble_loop

:hops
REM Max Hops
echo.
echo 7. Optional Parameters:
echo.
echo Max Hops:
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
REM Channel Name
:channel_loop
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
    goto :channel_loop
)

REM Convert to uppercase
call :to_upper CHANNEL

:password
REM Password
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

REM Convert to uppercase
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

REM Detect ports
call :detect_ports
if errorlevel 1 (
    echo ERROR: Port detection failed
    pause
    exit /b 1
)

REM Flash process
echo.
echo Starting flash process...
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
echo ===== CONFIGURATION SUMMARY =====
echo Device ID: !DEVICE_ID!
echo Role: !ROLE!
echo Transmission Interval: !INTERVAL! seconds
echo Region: !REGION!
echo Mode: !MODE!
echo Radio Profile: !RADIO!
echo Max Hops: !HOPS!
echo Network Channel: !CHANNEL!
if not "!PASSWORD!"=="" echo Network Password: !PASSWORD!
echo Port: !SELECTED_PORT!
echo.
echo Flash and configuration completed successfully!
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

REM Check length (simplified)
set "len=0"
:count_chars
if "!channel:~%len%,1!"=="" goto :len_done
set /a len+=1
goto :count_chars
:len_done

if !len! lss 3 exit /b 1
if !len! gtr 20 exit /b 1

REM Character validation using string operations instead of findstr
REM Check for spaces
if not "!channel!"=="!channel: =!" exit /b 1
REM Check for tabs (using a literal tab character)
if not "!channel!"=="!channel:	=!" exit /b 1
REM Check for forward slash
if not "!channel!"=="!channel:/=!" exit /b 1
REM Check for colon
if not "!channel!"=="!channel::=!" exit /b 1

REM Check reserved names
call :to_upper channel
for %%r in (CONFIG ADMIN DEBUG SYSTEM DEVICE LORA MESH TEST DEFAULT) do (
    if "!channel!"=="%%r" exit /b 1
)

exit /b 0

:validate_password
set "password=%~1"

REM Check length
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

REM Check if same as channel
call :to_upper password
if "!password!"=="!CHANNEL!" exit /b 1

exit /b 0

:check_dependencies
echo [SETUP] Checking dependencies...

REM Check Python first
set "PYTHON_CMD="
set "PIP_CMD="

REM Try different Python commands
for %%p in (python python3 py) do (
    %%p --version >nul 2>&1
    if not errorlevel 1 (
        set "PYTHON_CMD=%%p"
        set "PIP_CMD=%%p -m pip"
        goto :python_found
    )
)

echo ERROR: Python not found
echo.
echo PYTHON INSTALLATION INSTRUCTIONS:
echo 1. Go to https://python.org/downloads/
echo 2. Download Python 3.8 or newer for Windows
echo 3. Run the installer and IMPORTANT: Check "Add Python to PATH"
echo 4. Restart this tool after installation
echo.
echo Alternative: Install from Microsoft Store
echo 1. Open Microsoft Store
echo 2. Search for "Python 3.11" or newer
echo 3. Install the official Python package
echo.
pause
exit /b 1

:python_found
echo Python found: %PYTHON_CMD%

REM Check pip
%PIP_CMD% --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: pip not found
    echo Try: %PYTHON_CMD% -m ensurepip --upgrade
    pause
    exit /b 1
)

REM Check/install pyserial
%PYTHON_CMD% -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo Installing pyserial...
    %PIP_CMD% install pyserial
    if errorlevel 1 (
        echo ERROR: Failed to install pyserial
        echo Try running as Administrator or check internet connection
        pause
        exit /b 1
    )
    echo pyserial installed successfully
)

REM === PLATFORMIO DETECTION IMPROVED ===
echo.
echo Checking PlatformIO installation...

REM Method 1: Check if pio is in PATH
where pio >nul 2>&1
if not errorlevel 1 (
    set "PIO_CMD=pio"
    echo PlatformIO found in system PATH
    goto :pio_found
)

REM Method 2: Check user's .platformio directory
if exist "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" (
    set "PIO_CMD=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
    echo PlatformIO found in user directory
    goto :pio_found
)

REM Method 3: Check Python Scripts directory (pip install location)
for /f "delims=" %%i in ('%PYTHON_CMD% -c "import site; print(site.USER_BASE)"') do set "USER_BASE=%%i"
if exist "%USER_BASE%\Scripts\pio.exe" (
    set "PIO_CMD=%USER_BASE%\Scripts\pio.exe"
    echo PlatformIO found in Python user scripts
    goto :pio_found
)

REM Method 4: Check if installed as Python module
%PYTHON_CMD% -m platformio --version >nul 2>&1
if not errorlevel 1 (
    set "PIO_CMD=%PYTHON_CMD% -m platformio"
    echo PlatformIO found as Python module
    goto :pio_found
)

REM Method 5: Check local Python Scripts
for /f "delims=" %%i in ('%PYTHON_CMD% -c "import sys; import os; print(os.path.dirname(sys.executable))"') do set "PYTHON_DIR=%%i"
if exist "%PYTHON_DIR%\Scripts\pio.exe" (
    set "PIO_CMD=%PYTHON_DIR%\Scripts\pio.exe"
    echo PlatformIO found in Python installation directory
    goto :pio_found
)

REM If not found, try to install it
echo PlatformIO not found. Installing...
echo.
%PIP_CMD% install --upgrade --force-reinstall platformio
if errorlevel 1 (
    echo ERROR: Failed to install PlatformIO
    echo Try manually: %PIP_CMD% install --user platformio
    pause
    exit /b 1
)

REM After installation, try to find it again
timeout /t 3 /nobreak >nul

REM Re-check all locations after installation
for /f "delims=" %%i in ('%PYTHON_CMD% -c "import site; print(site.USER_BASE)"') do set "USER_BASE=%%i"
if exist "%USER_BASE%\Scripts\pio.exe" (
    set "PIO_CMD=%USER_BASE%\Scripts\pio.exe"
    echo PlatformIO installed successfully in user scripts
    goto :pio_found
)

REM Try as module
%PYTHON_CMD% -m platformio --version >nul 2>&1
if not errorlevel 1 (
    set "PIO_CMD=%PYTHON_CMD% -m platformio"
    echo PlatformIO installed as Python module
    goto :pio_found
)

echo WARNING: PlatformIO installed but not found in expected locations
echo Trying to use it as Python module...
set "PIO_CMD=%PYTHON_CMD% -m platformio"

:pio_found
echo.
echo Testing PlatformIO command: %PIO_CMD%
%PIO_CMD% --version
if errorlevel 1 (
    echo ERROR: PlatformIO command not working
    echo Please install manually and ensure it's in PATH
    pause
    exit /b 1
)

echo Dependencies checked successfully
exit /b 0

:detect_ports
echo.
echo ========================================
echo     ESP32 PORT DETECTION
echo ========================================
echo.

REM Create temporary Python script for port detection with improved diagnostics
set "temp_py=%TEMP%\detect_ports_%RANDOM%.py"
(
echo import serial.tools.list_ports
echo import sys
echo.
echo print^("Scanning for serial ports..."^)
echo print^("-" * 40^)
echo.
echo ports = list^(serial.tools.list_ports.comports^(^)^)
echo.
echo if not ports:
echo     print^("NO PORTS DETECTED AT ALL!"^)
echo     print^("This usually means:"^)
echo     print^("  1. No USB devices connected"^)
echo     print^("  2. Driver issues"^)
echo     print^("  3. Windows USB subsystem problem"^)
echo     sys.exit^(1^)
echo.
echo # Show all detected ports for debugging
echo print^(f"Found {len^(ports^)} total port^(s^):"^)
echo print^(^)
echo.
echo all_com_ports = []
echo for i, port in enumerate^(ports, 1^):
echo     print^(f"Port {i}:"^)
echo     print^(f"  Device: {port.device}"^)
echo     print^(f"  Description: {port.description}"^)
echo     print^(f"  Manufacturer: {port.manufacturer or 'Not specified'}"^)
echo     print^(f"  Hardware ID: {port.hwid}"^)
echo     print^(f"  VID:PID: {port.vid}:{port.pid if port.pid else 'N/A'}"^)
echo     print^(^)
echo     
echo     # Collect all COM ports
echo     if port.device.startswith^('COM'^):
echo         all_com_ports.append^(port^)
echo.
echo print^("-" * 40^)
echo.
echo # More inclusive ESP32 detection
echo esp32_candidates = []
echo.
echo for port in all_com_ports:
echo     device_lower = port.device.lower^(^)
echo     desc_lower = ^(port.description or ""^).lower^(^)
echo     hwid_lower = ^(port.hwid or ""^).lower^(^)
echo     manufacturer_lower = ^(port.manufacturer or ""^).lower^(^)
echo     
echo     # List of indicators that suggest ESP32/USB-Serial
echo     indicators = [
echo         "ch340", "ch341", "ch9102",  # Common Chinese chips
echo         "cp210", "cp2102", "cp2104",  # Silicon Labs
echo         "ft232", "ftdi",              # FTDI chips  
echo         "pl2303", "prolific",         # Prolific
echo         "esp32", "espressif",         # ESP specific
echo         "serial", "uart",             # Generic serial
echo         "usb-serial", "usb serial",   # Generic USB serial
echo         "arduino", "wch"              # Arduino/WCH chips
echo     ]
echo     
echo     # Check if any indicator is present
echo     is_likely_esp32 = False
echo     matched_indicator = ""
echo     
echo     for indicator in indicators:
echo         if ^(indicator in desc_lower or 
echo             indicator in hwid_lower or 
echo             indicator in manufacturer_lower^):
echo             is_likely_esp32 = True
echo             matched_indicator = indicator
echo             break
echo     
echo     # Also accept any COM port as potential ESP32 if no specific matches
echo     if not is_likely_esp32 and port.device.startswith^('COM'^):
echo         is_likely_esp32 = True
echo         matched_indicator = "generic COM port"
echo     
echo     if is_likely_esp32:
echo         esp32_candidates.append^(port^)
echo         print^(f"ESP32 Candidate: {port.device} ^(matched: {matched_indicator}^)"^)
echo.
echo print^(^)
echo print^("=" * 40^)
echo.
echo # Output decision for batch script
echo if not esp32_candidates:
echo     print^("NO_ESP32_FOUND"^)
echo     if all_com_ports:
echo         print^("But found these COM ports:"^)
echo         for port in all_com_ports:
echo             print^(f"  - {port.device}: {port.description}"^)
echo elif len^(esp32_candidates^) == 1:
echo     print^(f"SINGLE_ESP32:{esp32_candidates[0].device}"^)
echo else:
echo     print^("MULTIPLE_ESP32:"^)
echo     for i, port in enumerate^(esp32_candidates, 1^):
echo         print^(f"{i}:{port.device}:{port.description}"^)
) > "!temp_py!"

REM Run detection script
echo Running port detection script...
%PYTHON_CMD% "!temp_py!" 2>&1
echo.

REM Capture the last output line for decision
set "port_info="
for /f "delims=" %%i in ('%PYTHON_CMD% "!temp_py!" 2^>^&1 ^| findstr /B "SINGLE_ESP32: MULTIPLE_ESP32: NO_ESP32_FOUND"') do set "port_info=%%i"

REM Clean up temp file
del "!temp_py!" >nul 2>&1

REM Process detection results
if "!port_info!"=="" (
    echo ERROR: Port detection script failed
    echo Falling back to manual port entry...
    goto :manual_port_entry
)

echo !port_info! | findstr "NO_ESP32_FOUND" >nul
if not errorlevel 1 (
    echo.
    echo No ESP32 detected automatically.
    goto :manual_port_entry
)

echo !port_info! | findstr "SINGLE_ESP32:" >nul
if not errorlevel 1 (
    for /f "tokens=2 delims=:" %%a in ("!port_info!") do set "SELECTED_PORT=%%a"
    echo.
    echo SUCCESS: ESP32 detected on !SELECTED_PORT!
    echo.
    exit /b 0
)

echo !port_info! | findstr "MULTIPLE_ESP32:" >nul
if not errorlevel 1 (
    echo Multiple ESP32 devices detected.
    echo Please disconnect all but one ESP32 and try again.
    goto :manual_port_entry
)

:manual_port_entry
echo.
echo ========================================
echo     MANUAL PORT SELECTION
echo ========================================
echo.
echo The automatic detection couldn't find your ESP32.
echo.
echo Common reasons:
echo   1. Driver not installed (CH340, CP2102, etc.)
echo   2. Bad USB cable (some are power-only)
echo   3. ESP32 not in bootloader mode
echo   4. Port already in use by another program
echo.
echo Check Device Manager (Win+X, then M) for COM ports.
echo Your ESP32 should appear under "Ports (COM & LPT)"
echo.
set /p "manual_port=Enter your COM port (e.g., COM3, COM4): "

if "!manual_port!"=="" (
    echo No port entered. Exiting.
    exit /b 1
)

REM Validate manual port entry
echo !manual_port! | findstr /R "^COM[0-9][0-9]*$" >nul
if errorlevel 1 (
    echo ERROR: Invalid port format. Use format like COM3, COM4, etc.
    exit /b 1
)

echo.
echo Testing connection to !manual_port!...

REM Test if port can be opened
set "test_py=%TEMP%\test_port_%RANDOM%.py"
(
echo import serial
echo import sys
echo try:
echo     ser = serial.Serial^('!manual_port!', 115200, timeout=1^)
echo     print^(f"SUCCESS: Port {ser.name} opened successfully"^)
echo     ser.close^(^)
echo except serial.SerialException as e:
echo     print^(f"ERROR: Cannot open !manual_port!"^)
echo     print^(f"Reason: {e}"^)
echo     sys.exit^(1^)
echo except Exception as e:
echo     print^(f"ERROR: {e}"^)
echo     sys.exit^(1^)
) > "!test_py!"

%PYTHON_CMD% "!test_py!"
if errorlevel 1 (
    del "!test_py!" >nul 2>&1
    echo.
    echo Failed to open !manual_port!
    echo Please check:
    echo   - Port number is correct
    echo   - No other program is using the port
    echo   - USB cable is connected properly
    exit /b 1
)

del "!test_py!" >nul 2>&1

set "SELECTED_PORT=!manual_port!"
echo Using manual port: !SELECTED_PORT!
exit /b 0

:clean_device_flash
echo.
echo [1/4] Cleaning device flash...
cd /d "!PROJECT_DIR!"

echo Executing: "!PIO_CMD!" run -e !BOARD_TYPE! --target erase --upload-port !SELECTED_PORT!
"!PIO_CMD!" run -e "!BOARD_TYPE!" --target erase --upload-port "!SELECTED_PORT!"
if errorlevel 1 (
    echo Warning: Device cleaning failed, proceeding anyway
)
echo Device flash cleaned successfully
exit /b 0

:flash_firmware
echo.
echo [2/4] Flashing firmware...
cd /d "!PROJECT_DIR!"

REM Small delay after erase
timeout /t 2 /nobreak >nul

echo Executing: "!PIO_CMD!" run -e !BOARD_TYPE! --target upload --upload-port !SELECTED_PORT!
"!PIO_CMD!" run -e "!BOARD_TYPE!" --target upload --upload-port "!SELECTED_PORT!"
if errorlevel 1 (
    echo Firmware flash failed
    call :print_manual_flash_instructions
    exit /b 1
)

echo Firmware flashed successfully
exit /b 0

:print_manual_flash_instructions
echo.
echo Manual solution:
echo 1. Close any open serial monitors
echo 2. Hold BOOT button on ESP32
echo 3. Press and release RESET button
echo 4. Release BOOT button
echo 5. Run these commands manually:
echo    cd "!PROJECT_DIR!"
echo    "!PIO_CMD!" run -e !BOARD_TYPE! --target erase --upload-port !SELECTED_PORT!
echo    "!PIO_CMD!" run -e !BOARD_TYPE! --target upload --upload-port !SELECTED_PORT!
exit /b 0

:configure_device
echo.
echo [3/4] Configuring device...

REM Wait for device reboot
timeout /t 8 /nobreak >nul

REM Re-detect port (may have changed after flash)
call :detect_ports
if errorlevel 1 (
    echo WARNING: Cannot detect port after flash
    call :print_manual_config
    exit /b 1
)

echo Using configuration port: !SELECTED_PORT!

REM Convert strings to uppercase
call :to_upper CHANNEL
call :to_upper PASSWORD
call :to_upper ROLE
call :to_upper REGION
call :to_upper MODE
call :to_upper RADIO

set "network_cmd=NETWORK_CREATE !CHANNEL! !PASSWORD!"
set "config_cmd="

if /i "!RADIO!"=="CUSTOM_ADVANCED" (
    set "config_cmd=Q_CONFIG !ROLE!,!DEVICE_ID!,!INTERVAL!,!REGION!,!MODE!,!RADIO!,!HOPS!,!CUSTOM_BW!,!CUSTOM_SF!,!CUSTOM_CR!,!CUSTOM_POWER!,!CUSTOM_PREAMBLE!"
) else (
    set "config_cmd=Q_CONFIG !ROLE!,!DEVICE_ID!,!INTERVAL!,!REGION!,!MODE!,!RADIO!,!HOPS!"
)

REM Create temporary Python script for serial communication
set "temp_py=%TEMP%\configure_device_%RANDOM%.py"
(
echo import serial
echo import time
echo import sys
echo.
echo try:
echo     ser = serial.Serial^('!SELECTED_PORT!', 115200, timeout=15^)
echo     time.sleep^(5^)  # Wait for device initialization
echo.    
echo     print^('Clearing input buffer...'ط)
echo     ser.reset_input_buffer^(^)
echo.    
echo     # Test communication
echo     print^('Testing device communication...'ط)
echo     ser.write^(b'\r\n'ط)
echo     time.sleep^(1^)
echo     ser.write^(b'STATUS\r\n'ط)
echo     time.sleep^(2^)
echo.    
echo     # Clear status response
echo     while ser.in_waiting ^> 0:
echo         ser.read^(ser.in_waiting^)
echo         time.sleep^(0.1^)
echo.    
echo     print^('Device communication established'ط)
echo.    
echo     # Step 1: Create network
echo     print^('Creating network: !CHANNEL!'ط)
echo     ser.write^(b'!network_cmd!\r\n'ط)
echo     time.sleep^(5^)
echo.    
echo     # Step 2: Configure device
echo     print^('Configuring device...'ط)
echo     ser.write^(b'!config_cmd!\r\n'ط)
echo     time.sleep^(5^)
echo.    
echo     # Step 3: Save configuration
echo     print^('Saving configuration...')
echo     ser.write^(b'CONFIG_SAVE\r\n')
echo     time.sleep^(3^)
echo.    
echo     # Step 4: Start device
echo     print^('Starting device...')
echo     ser.write^(b'START\r\n')
echo     time.sleep^(3^)
echo.    
echo     # Step 5: Check network status
echo     print^('Checking network status...')
echo     ser.write^(b'NETWORK_STATUS\r\n')
echo     time.sleep^(3^)
echo.    
echo     # Read final response
echo     response = ""
echo     start_time = time.time^(^)
echo     while time.time^(^) - start_time ^< 5:
echo         if ser.in_waiting ^> 0:
echo             response += ser.read^(ser.in_waiting^).decode^('utf-8', errors='ignore')
echo             time.sleep^(0.1^)
echo         else:
echo             time.sleep^(0.1^)
echo.    
echo     print^(f'Final response: {response}')
echo.    
echo     ser.close^(^)
echo     print^('CONFIGURATION_SUCCESS')
echo.    
echo except Exception as e:
echo     print^(f'CONFIGURATION_ERROR: {e}')
echo     import traceback
echo     traceback.print_exc^(^)
echo     sys.exit^(1^)
) > "!temp_py!"

REM Run Python script
%PYTHON_CMD% "!temp_py!" 2>nul
set "config_result=!errorlevel!"
del "!temp_py!" >nul 2>&1

if !config_result! neq 0 (
    echo Configuration failed
    call :print_manual_config
    exit /b 1
)

echo Device configured successfully
exit /b 0

:print_manual_config
echo.
echo === MANUAL RECOVERY ===
echo 1. Connect manually: "!PIO_CMD!" device monitor --port !SELECTED_PORT! --baud 115200
echo 2. Send these commands one by one:
echo    STATUS
echo    !network_cmd!
echo    !config_cmd!
echo    CONFIG_SAVE
echo    START
echo    NETWORK_STATUS
exit /b 0

:launch_monitor
echo.
echo [4/4] Launching serial monitor...
echo.
echo Manual command to monitor device:
echo "!PIO_CMD!" device monitor --baud 115200 --port !SELECTED_PORT!
echo.
echo Opening command prompt with monitor...
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