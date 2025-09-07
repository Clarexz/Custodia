#!/bin/bash

# Custodia Flash Tool for macOS
# Shell script version of the Python flash tool
# Automatically opens in Terminal when executed

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Global variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PIO_CMD=""
SELECTED_PORT=""
BOARD_TYPE="seeed_xiao_esp32s3"

# Custom advanced radio parameters
CUSTOM_BW=""
CUSTOM_SF=""
CUSTOM_CR=""
CUSTOM_POWER=""
CUSTOM_PREAMBLE=""

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_header() {
    clear
    print_color $BLUE "=================================="
    print_color $BLUE "    Custodia Flash Tool v2.0     "
    print_color $BLUE "        macOS Terminal Version    "
    print_color $BLUE "=================================="
    echo
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install Python if needed
install_python() {
    if ! command_exists python3; then
        print_color $YELLOW "Python 3 not found. Attempting installation..."
        
        # Check if Homebrew is available
        if command_exists brew; then
            print_color $YELLOW "Installing Python via Homebrew..."
            if brew install python; then
                print_color $GREEN "Python installed successfully via Homebrew"
                return 0
            else
                print_color $RED "Failed to install Python via Homebrew"
            fi
        fi
        
        # If Homebrew fails or isn't available
        print_color $YELLOW "Please install Python manually:"
        print_color $YELLOW "1. Download from https://python.org"
        print_color $YELLOW "2. Or install Homebrew: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        print_color $YELLOW "3. Then run: brew install python"
        return 1
    fi
    
    return 0
}

# Function to install dependencies
install_dependencies() {
    print_color $YELLOW "[SETUP] Checking dependencies..."
    
    # Install Python if needed
    if ! install_python; then
        return 1
    fi
    
    # Check pip
    if ! command_exists pip3; then
        print_color $RED "ERROR: pip3 not found"
        print_color $YELLOW "Try: python3 -m ensurepip --upgrade"
        return 1
    fi
    
    # Check/install pyserial
    if ! python3 -c "import serial" 2>/dev/null; then
        print_color $YELLOW "Installing pyserial..."
        if pip3 install pyserial; then
            print_color $GREEN "pyserial installed successfully"
        else
            print_color $RED "Failed to install pyserial"
            return 1
        fi
    else
        print_color $GREEN "pyserial already installed"
    fi
    
    return 0
}

# Function to find PlatformIO
find_platformio() {
    # Check if pio is in PATH
    if command_exists pio; then
        if pio --version >/dev/null 2>&1; then
            echo "pio"
            return 0
        fi
    fi
    
    # Check common installation paths
    local possible_paths=(
        "$HOME/.local/bin/pio"
        "$HOME/.platformio/penv/bin/pio"
        "/usr/local/bin/pio"
        "/opt/homebrew/bin/pio"
    )
    
    for path in "${possible_paths[@]}"; do
        if [[ -f "$path" ]] && "$path" --version >/dev/null 2>&1; then
            echo "$path"
            return 0
        fi
    done
    
    return 1
}

# Function to install PlatformIO
install_platformio() {
    print_color $YELLOW "PlatformIO not found. Installing globally..."
    
    # Try to install globally first
    if pip3 install --user platformio; then
        print_color $GREEN "PlatformIO installed successfully (user)"
        
        # Add to PATH for current session
        export PATH="$PATH:$HOME/.local/bin"
        
        # Add to shell profile for future sessions
        local shell_profile=""
        if [[ "$SHELL" == */zsh ]]; then
            shell_profile="$HOME/.zshrc"
        elif [[ "$SHELL" == */bash ]]; then
            shell_profile="$HOME/.bash_profile"
        fi
        
        if [[ -n "$shell_profile" ]]; then
            if ! grep -q "/.local/bin" "$shell_profile" 2>/dev/null; then
                echo 'export PATH="$PATH:$HOME/.local/bin"' >> "$shell_profile"
                print_color $BLUE "Added PlatformIO to PATH in $shell_profile"
            fi
        fi
        
        sleep 2
        
        # Try to find it again
        PIO_CMD=$(find_platformio)
        if [[ -n "$PIO_CMD" ]]; then
            print_color $GREEN "PlatformIO ready: $PIO_CMD"
            return 0
        else
            print_color $YELLOW "Installation completed but PlatformIO not immediately available"
            print_color $YELLOW "Please restart your terminal and try again"
            return 1
        fi
    else
        print_color $RED "Failed to install PlatformIO"
        print_color $YELLOW "Manual installation:"
        print_color $YELLOW "1. pip3 install --user platformio"
        print_color $YELLOW "2. Or: brew install platformio"
        print_color $YELLOW "3. Add ~/.local/bin to your PATH"
        return 1
    fi
}

# Function to verify project structure
verify_project_structure() {
    print_color $BLUE "Verifying project structure..."
    
    # Check if we can access the project directory
    if [[ ! -d "$PROJECT_DIR" ]]; then
        print_color $RED "ERROR: Project directory not found: $PROJECT_DIR"
        return 1
    fi
    
    # Check for platformio.ini
    if [[ ! -f "$PROJECT_DIR/platformio.ini" ]]; then
        print_color $RED "ERROR: platformio.ini not found in project directory"
        print_color $YELLOW "Expected location: $PROJECT_DIR/platformio.ini"
        return 1
    fi
    
    # Check for src directory
    if [[ ! -d "$PROJECT_DIR/src" ]]; then
        print_color $RED "ERROR: src directory not found in project"
        print_color $YELLOW "Expected location: $PROJECT_DIR/src"
        return 1
    fi
    
    print_color $GREEN "Project structure verified successfully"
    print_color $BLUE "Project directory: $PROJECT_DIR"
    print_color $BLUE "Script directory: $SCRIPT_DIR"
    return 0
}

# Function to detect ESP32 devices
detect_ports() {
    print_color $YELLOW "Detecting ESP32 devices..."
    
    # Use Python to detect ports (more reliable than shell parsing)
    local port_script='
import serial.tools.list_ports
import sys

ports = serial.tools.list_ports.comports()
esp32_ports = []

for port in ports:
    device_lower = port.device.lower()
    desc_lower = port.description.lower()
    manufacturer_lower = (port.manufacturer or "").lower()
    
    # Skip non-relevant devices
    skip_patterns = ["bluetooth", "debug-console", "hub", "keyboard", "mouse"]
    if any(pattern in desc_lower for pattern in skip_patterns):
        continue
    
    # ESP32 indicators
    esp32_indicators = ["cp210", "ch340", "ch341", "ft232", "pl2303", "esp32", "serial", "uart", "usb-serial", "ttyusb", "ttyacm", "usbserial", "usbmodem"]
    
    is_esp32_candidate = False
    
    if any(pattern in device_lower for pattern in ["ttyusb", "ttyacm", "usbserial", "usbmodem"]):
        is_esp32_candidate = True
    
    if any(indicator in desc_lower for indicator in esp32_indicators):
        is_esp32_candidate = True
    
    if any(mfg in manufacturer_lower for mfg in ["silicon labs", "ftdi", "prolific", "ch340"]):
        is_esp32_candidate = True
    
    if is_esp32_candidate:
        esp32_ports.append((port.device, port.description))

if not esp32_ports:
    print("NO_ESP32_FOUND")
    if ports:
        print("AVAILABLE_PORTS:")
        for i, port in enumerate(ports, 1):
            print(f"{i}:{port.device}:{port.description}")
else:
    if len(esp32_ports) == 1:
        print(f"SINGLE_ESP32:{esp32_ports[0][0]}:{esp32_ports[0][1]}")
    else:
        print("MULTIPLE_ESP32:")
        for i, (device, desc) in enumerate(esp32_ports, 1):
            print(f"{i}:{device}:{desc}")
'
    
    local port_info=$(python3 -c "$port_script" 2>/dev/null)
    
    if [[ -z "$port_info" ]]; then
        print_color $RED "ERROR: Could not detect ports (pyserial issue)"
        return 1
    fi
    
    if [[ "$port_info" == "NO_ESP32_FOUND" ]]; then
        print_color $YELLOW "No ESP32 devices detected automatically."
        return 1
    elif [[ "$port_info" =~ ^SINGLE_ESP32: ]]; then
        local device=$(echo "$port_info" | cut -d: -f2)
        local description=$(echo "$port_info" | cut -d: -f3-)
        print_color $GREEN "ESP32 device detected: $device - $description"
        SELECTED_PORT="$device"
        return 0
    elif [[ "$port_info" =~ ^MULTIPLE_ESP32: ]]; then
        print_color $YELLOW "Multiple ESP32 devices found:"
        local ports_array=()
        while IFS= read -r line; do
            if [[ "$line" =~ ^[0-9]+: ]]; then
                local num=$(echo "$line" | cut -d: -f1)
                local device=$(echo "$line" | cut -d: -f2)
                local desc=$(echo "$line" | cut -d: -f3-)
                echo "  $num. $device - $desc"
                ports_array+=("$device")
            fi
        done <<< "$port_info"
        
        echo
        while true; do
            read -p "Select port [1-${#ports_array[@]}]: " choice
            if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#ports_array[@]}" ]; then
                SELECTED_PORT="${ports_array[$((choice-1))]}"
                print_color $GREEN "Selected: $SELECTED_PORT"
                return 0
            else
                print_color $RED "Invalid selection. Please enter a number between 1 and ${#ports_array[@]}"
            fi
        done
    fi
    
    return 1
}

# Function to detect board type (only XIAO ESP32S3 now)
detect_board_type() {
    BOARD_TYPE="seeed_xiao_esp32s3"
    print_color $BLUE "Using board: XIAO ESP32S3"
}

# Function to validate role
validate_role() {
    local role="$1"
    local role_upper=$(echo "$role" | tr '[:lower:]' '[:upper:]')
    local valid_roles=("TRACKER" "RECEIVER" "REPEATER")
    
    for valid_role in "${valid_roles[@]}"; do
        if [[ "$role_upper" == "$valid_role" ]]; then
            return 0
        fi
    done
    return 1
}

# Function to get role with validation
get_role() {
    while true; do
        echo
        print_color $BLUE "Device Role:"
        echo "  1. TRACKER  - GPS tracking device"
        echo "  2. RECEIVER - Base station/receiver"
        echo "  3. REPEATER - Signal repeater/relay"
        echo
        read -p "Select role [1-3]: " choice
        
        case $choice in
            1) ROLE="TRACKER"; break ;;
            2) ROLE="RECEIVER"; break ;;
            3) ROLE="REPEATER"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1, 2, or 3" ;;
        esac
    done
}

# Function to validate device ID
validate_device_id() {
    local id="$1"
    if [[ "$id" =~ ^[0-9]+$ ]] && [ "$id" -ge 1 ] && [ "$id" -le 255 ]; then
        return 0
    fi
    return 1
}

# Function to get device ID with validation
get_device_id() {
    while true; do
        echo
        print_color $BLUE "Device ID:"
        echo "  Unique identifier for this device (1-255)"
        echo
        read -p "Enter device ID [1-255]: " DEVICE_ID
        
        if validate_device_id "$DEVICE_ID"; then
            break
        else
            print_color $RED "Invalid device ID. Please enter a number between 1 and 255"
        fi
    done
}

# Function to validate interval
validate_interval() {
    local interval="$1"
    if [[ "$interval" =~ ^[0-9]+$ ]] && [ "$interval" -ge 5 ] && [ "$interval" -le 3600 ]; then
        return 0
    fi
    return 1
}

# Function to get interval with validation
get_interval() {
    while true; do
        echo
        print_color $BLUE "Transmission Interval:"
        echo "  How often device sends GPS updates (in seconds)"
        echo "  Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)"
        echo
        read -p "Enter interval in seconds [5-3600]: " INTERVAL
        
        if validate_interval "$INTERVAL"; then
            break
        else
            print_color $RED "Invalid interval. Please enter a number between 5 and 3600 seconds"
        fi
    done
}

# Function to get region with validation
get_region() {
    while true; do
        echo
        print_color $BLUE "LoRa Region:"
        echo "  1. US - United States"
        echo "  2. EU - Europe"
        echo "  3. CH - China"
        echo "  4. AS - Asia"
        echo "  5. JP - Japan"
        echo
        read -p "Select region [1-5]: " choice
        
        case $choice in
            1) REGION="US"; break ;;
            2) REGION="EU"; break ;;
            3) REGION="CH"; break ;;
            4) REGION="AS"; break ;;
            5) REGION="JP"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1, 2, 3, 4, or 5" ;;
        esac
    done
}

# Function to get mode with validation
get_mode() {
    while true; do
        echo
        print_color $BLUE "Operation Mode:"
        echo "  1. SIMPLE - Basic operation"
        echo "  2. ADMIN  - Advanced features enabled"
        echo
        read -p "Select mode [1-2]: " choice
        
        case $choice in
            1) MODE="SIMPLE"; break ;;
            2) MODE="ADMIN"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1 or 2" ;;
        esac
    done
}

# Function to get custom advanced parameters
get_custom_advanced_params() {
    echo
    print_color $YELLOW "Custom Advanced Radio Parameters:"
    echo
    
    # Bandwidth
    while true; do
        print_color $BLUE "Bandwidth (kHz):"
        echo "  1. 125 kHz (default)"
        echo "  2. 250 kHz"
        echo "  3. 500 kHz"
        echo
        read -p "Select bandwidth [1-3]: " choice
        
        case $choice in
            1) CUSTOM_BW="125"; break ;;
            2) CUSTOM_BW="250"; break ;;
            3) CUSTOM_BW="500"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1, 2, or 3" ;;
        esac
    done
    
    # Spreading Factor
    while true; do
        print_color $BLUE "Spreading Factor:"
        echo "  1. SF7  (fastest, shortest range)"
        echo "  2. SF8"
        echo "  3. SF9"
        echo "  4. SF10"
        echo "  5. SF11"
        echo "  6. SF12 (slowest, longest range)"
        echo
        read -p "Select spreading factor [1-6]: " choice
        
        case $choice in
            1) CUSTOM_SF="7"; break ;;
            2) CUSTOM_SF="8"; break ;;
            3) CUSTOM_SF="9"; break ;;
            4) CUSTOM_SF="10"; break ;;
            5) CUSTOM_SF="11"; break ;;
            6) CUSTOM_SF="12"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1-6" ;;
        esac
    done
    
    # Coding Rate
    while true; do
        print_color $BLUE "Coding Rate:"
        echo "  1. 4/5 (fastest)"
        echo "  2. 4/6"
        echo "  3. 4/7"
        echo "  4. 4/8 (most robust)"
        echo
        read -p "Select coding rate [1-4]: " choice
        
        case $choice in
            1) CUSTOM_CR="5"; break ;;
            2) CUSTOM_CR="6"; break ;;
            3) CUSTOM_CR="7"; break ;;
            4) CUSTOM_CR="8"; break ;;
            *) print_color $RED "Invalid selection. Please enter 1-4" ;;
        esac
    done
    
    # TX Power
    while true; do
        print_color $BLUE "TX Power (dBm):"
        echo "  Range: 2-22 dBm"
        echo "  Recommended: 14-20 dBm"
        echo
        read -p "Enter TX power [2-22]: " CUSTOM_POWER
        
        if [[ "$CUSTOM_POWER" =~ ^[0-9]+$ ]] && [ "$CUSTOM_POWER" -ge 2 ] && [ "$CUSTOM_POWER" -le 22 ]; then
            break
        else
            print_color $RED "Invalid power. Please enter a number between 2 and 22"
        fi
    done
    
    # Preamble Length
    while true; do
        print_color $BLUE "Preamble Length:"
        echo "  Range: 6-65535"
        echo "  Default: 8"
        echo
        read -p "Enter preamble length [6-65535]: " CUSTOM_PREAMBLE
        
        if [[ "$CUSTOM_PREAMBLE" =~ ^[0-9]+$ ]] && [ "$CUSTOM_PREAMBLE" -ge 6 ] && [ "$CUSTOM_PREAMBLE" -le 65535 ]; then
            break
        else
            print_color $RED "Invalid preamble length. Please enter a number between 6 and 65535"
        fi
    done
}

# Function to get radio profile with validation
get_radio() {
    while true; do
        echo
        print_color $BLUE "Radio Profile:"
        echo "  1. DESERT_LONG_FAST  - Long range, fast transmission"
        echo "  2. MOUNTAIN_STABLE   - Stable connection in mountains"
        echo "  3. URBAN_DENSE       - Dense urban environments"
        echo "  4. MESH_MAX_NODES    - Maximum mesh network nodes"
        echo "  5. CUSTOM_ADVANCED   - Custom advanced settings"
        echo
        read -p "Select radio profile [1-5]: " choice
        
        case $choice in
            1) RADIO="DESERT_LONG_FAST"; break ;;
            2) RADIO="MOUNTAIN_STABLE"; break ;;
            3) RADIO="URBAN_DENSE"; break ;;
            4) RADIO="MESH_MAX_NODES"; break ;;
            5) RADIO="CUSTOM_ADVANCED"; get_custom_advanced_params; break ;;
            *) print_color $RED "Invalid selection. Please enter 1-5" ;;
        esac
    done
}

# Function to validate hops
validate_hops() {
    local hops="$1"
    if [[ "$hops" =~ ^[0-9]+$ ]] && [ "$hops" -ge 1 ] && [ "$hops" -le 10 ]; then
        return 0
    fi
    return 1
}

# Function to get hops with validation
get_hops() {
    while true; do
        echo
        print_color $BLUE "Max Hops:"
        echo "  Maximum number of hops for mesh network (1-10)"
        echo "  Default: 3"
        echo
        read -p "Enter max hops [1-10] or press Enter for default (3): " input
        
        if [[ -z "$input" ]]; then
            HOPS=3
            break
        elif validate_hops "$input"; then
            HOPS="$input"
            break
        else
            print_color $RED "Invalid hops. Please enter a number between 1 and 10"
        fi
    done
}

# Function to validate channel name
validate_channel() {
    local channel=$(echo "$1" | tr '[:lower:]' '[:upper:]')
    
    # Check length
    if [ ${#channel} -lt 3 ] || [ ${#channel} -gt 20 ]; then
        return 1
    fi
    
    # Check characters (only alphanumeric and underscore)
    if [[ ! "$channel" =~ ^[A-Z0-9_]+$ ]]; then
        return 1
    fi
    
    # Check reserved names
    local reserved_names=("CONFIG" "ADMIN" "DEBUG" "SYSTEM" "DEVICE" "LORA" "MESH" "TEST" "DEFAULT")
    for reserved in "${reserved_names[@]}"; do
        if [[ "$channel" == "$reserved" ]]; then
            return 1
        fi
    done
    
    return 0
}

# Function to get channel with validation
get_channel() {
    while true; do
        echo
        print_color $BLUE "Network Channel:"
        echo "  Network name for your devices (3-20 characters)"
        echo "  Only letters, numbers, and underscore allowed"
        echo "  Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS"
        echo
        read -p "Enter channel name: " CHANNEL
        
        if validate_channel "$CHANNEL"; then
            CHANNEL=$(echo "$CHANNEL" | tr '[:lower:]' '[:upper:]')
            break
        else
            print_color $RED "Invalid channel name. Requirements:"
            print_color $RED "  - 3-20 characters"
            print_color $RED "  - Only letters, numbers, underscore"
            print_color $RED "  - Cannot be: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT"
        fi
    done
}

# Function to validate password
validate_password() {
    local password=$(echo "$1" | tr '[:lower:]' '[:upper:]')
    
    # Check length
    if [ ${#password} -lt 8 ] || [ ${#password} -gt 32 ]; then
        return 1
    fi
    
    # Check for at least one number and one letter
    if [[ ! "$password" =~ [0-9] ]] || [[ ! "$password" =~ [A-Z] ]]; then
        return 1
    fi
    
    # Check if same as channel
    local channel_upper=$(echo "$CHANNEL" | tr '[:lower:]' '[:upper:]')
    if [[ "$password" == "$channel_upper" ]]; then
        return 1
    fi
    
    return 0
}

# Function to generate password
generate_password() {
    local chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    local password=""
    
    # Ensure at least one letter and one number
    password+=$(echo "ABCDEFGHIJKLMNOPQRSTUVWXYZ" | fold -w1 | sort -R | head -n1)
    password+=$(echo "0123456789" | fold -w1 | sort -R | head -n1)
    
    # Fill remaining 6 characters
    for i in {1..6}; do
        password+=$(echo "$chars" | fold -w1 | sort -R | head -n1)
    done
    
    # Shuffle the password
    PASSWORD=$(echo "$password" | fold -w1 | sort -R | tr -d '\n')
}

# Function to get password with validation
get_password() {
    while true; do
        echo
        print_color $BLUE "Network Password:"
        echo "  Password requirements:"
        echo "  - 8-32 characters"
        echo "  - Must contain at least 1 number and 1 letter"
        echo "  - Cannot be the same as channel name"
        echo "  Examples: Secure123, MyPass99, Field2024"
        echo
        read -p "Enter password or press Enter for auto-generation: " input
        
        if [[ -z "$input" ]]; then
            generate_password
            print_color $GREEN "Password auto-generated: $PASSWORD"
            break
        elif validate_password "$input"; then
            PASSWORD=$(echo "$input" | tr '[:lower:]' '[:upper:]')
            break
        else
            print_color $RED "Invalid password. Requirements:"
            print_color $RED "  - 8-32 characters"
            print_color $RED "  - At least 1 number and 1 letter"
            print_color $RED "  - Cannot be same as channel name"
        fi
    done
}

# Function to calculate network hash
calculate_network_hash() {
    local hash=0
    
    local combined_upper=$(echo "${CHANNEL}${PASSWORD}" | tr '[:lower:]' '[:upper:]')
    for (( i=0; i<${#combined_upper}; i++ )); do
        local char="${combined_upper:$i:1}"
        local ascii=$(printf "%d" "'$char")
        hash=$(( (hash * 31 + ascii) & 0xFFFFFFFF ))
    done
    
    printf "%08X" $hash
}

# Function for one-line configuration
get_oneline_config() {
    print_color $BLUE "One-line Configuration Mode"
    echo
    print_color $YELLOW "Example:"
    print_color $GREEN "./flash_tool_macos.sh -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3"
    echo
    print_color $YELLOW "Available parameters:"
    echo "Available parameters:"
    print_color $BLUE "  -role [TRACKER|RECEIVER|REPEATER]"
    print_color $BLUE "  -id [1-255]"
    print_color $BLUE "  -interval [5-3600]"
    print_color $BLUE "  -region [US|EU|CH|AS|JP]"
    print_color $BLUE "  -mode [SIMPLE|ADMIN]"
    print_color $BLUE "  -radio [DESERT_LONG_FAST|MOUNTAIN_STABLE|URBAN_DENSE|MESH_MAX_NODES]"
    print_color $BLUE "  -channel [channel_name]"
    print_color $BLUE "  -password [password] (optional)"
    print_color $BLUE "  -hops [1-10] (optional, default: 3)"
    echo
    
    read -p "Enter your configuration command: " config_line
    
    # Parse the configuration line
    eval "set -- $config_line"
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -role)
                ROLE="$2"
                shift 2
                ;;
            -id)
                DEVICE_ID="$2"
                shift 2
                ;;
            -interval)
                INTERVAL="$2"
                shift 2
                ;;
            -region)
                REGION="$2"
                shift 2
                ;;
            -mode)
                MODE="$2"
                shift 2
                ;;
            -radio)
                RADIO="$2"
                shift 2
                ;;
            -hops)
                HOPS="$2"
                shift 2
                ;;
            -channel)
                CHANNEL="$2"
                shift 2
                ;;
            -password)
                PASSWORD="$2"
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done
    
    # Set defaults
    HOPS=${HOPS:-3}
    
    # Generate password if not provided
    if [[ -z "$PASSWORD" ]]; then
        generate_password
        print_color $GREEN "Password auto-generated: $PASSWORD"
    fi
}

# Function to get configuration type
get_config_type() {
    while true; do
        echo
        print_color $BLUE "Configuration Type:"
        echo "  1. Normal     - Interactive step-by-step configuration"
        echo "  2. One Line   - Single command line configuration"
        echo
        read -p "Select configuration type [1-2]: " choice
        
        case $choice in
            1) return 0 ;;  # Normal mode
            2) return 1 ;;  # One-line mode
            *) print_color $RED "Invalid selection. Please enter 1 or 2" ;;
        esac
    done
}

# Function to validate all parameters
validate_all_parameters() {
    local errors=()
    
    # Validate role
    if ! validate_role "$ROLE"; then
        errors+=("Invalid role: '$ROLE'. Valid values: TRACKER, RECEIVER, REPEATER")
    fi
    
    # Validate device ID
    if ! validate_device_id "$DEVICE_ID"; then
        errors+=("Invalid device ID: '$DEVICE_ID'. Valid range: 1-255")
    fi
    
    # Validate interval
    if ! validate_interval "$INTERVAL"; then
        errors+=("Invalid interval: '$INTERVAL'. Valid range: 5-3600 seconds")
    fi
    
    # Validate region
    local valid_regions=("US" "EU" "CH" "AS" "JP")
    local region_upper=$(echo "$REGION" | tr '[:lower:]' '[:upper:]')
    if [[ ! " ${valid_regions[@]} " =~ " $region_upper " ]]; then
        errors+=("Invalid region: '$REGION'. Valid values: ${valid_regions[*]}")
    fi
    
    # Validate mode
    local valid_modes=("SIMPLE" "ADMIN")
    local mode_upper=$(echo "$MODE" | tr '[:lower:]' '[:upper:]')
    if [[ ! " ${valid_modes[@]} " =~ " $mode_upper " ]]; then
        errors+=("Invalid mode: '$MODE'. Valid values: ${valid_modes[*]}")
    fi
    
    # Validate radio profile
    local valid_radios=("DESERT_LONG_FAST" "MOUNTAIN_STABLE" "URBAN_DENSE" "MESH_MAX_NODES" "CUSTOM_ADVANCED")
    local radio_upper=$(echo "$RADIO" | tr '[:lower:]' '[:upper:]')
    if [[ ! " ${valid_radios[@]} " =~ " $radio_upper " ]]; then
        errors+=("Invalid radio profile: '$RADIO'. Valid values: ${valid_radios[*]}")
    fi
    
    # Validate hops
    if ! validate_hops "$HOPS"; then
        errors+=("Invalid hops: '$HOPS'. Valid range: 1-10")
    fi
    
    # Validate channel
    if ! validate_channel "$CHANNEL"; then
        errors+=("Invalid channel name: '$CHANNEL'")
    fi
    
    # Validate password
    if ! validate_password "$PASSWORD"; then
        errors+=("Invalid password: '$PASSWORD'")
    fi
    
    # Print errors if any
    if [ ${#errors[@]} -gt 0 ]; then
        print_color $RED "CONFIGURATION ERRORS:"
        echo
        for error in "${errors[@]}"; do
            print_color $RED "  $error"
        done
        echo
        return 1
    fi
    
    return 0
}

# Function to clean device flash completely
clean_device_flash() {
    print_color $YELLOW "\n[1/4] Cleaning device flash..."
    print_color $BLUE "Erasing all flash memory and configurations..."
    print_color $BLUE "This ensures no previous configurations interfere with new setup"
    
    # Method 1: Try PlatformIO erase target
    print_color $BLUE "Attempting complete flash erase..."
    cd "$PROJECT_DIR" || {
        print_color $RED "Cannot change to project directory: $PROJECT_DIR"
        return 1
    }
    
    # Verify we're in the right directory
    if [[ ! -f "platformio.ini" ]]; then
        print_color $RED "ERROR: Not in PlatformIO project directory"
        print_color $YELLOW "Current directory: $(pwd)"
        return 1
    fi
    
    if "$PIO_CMD" run -e "$BOARD_TYPE" --target erase --upload-port "$SELECTED_PORT" 2>&1; then
        print_color $GREEN "Complete flash erase successful"
        sleep 3  # Allow time for erase to settle
        return 0
    fi
    
    # Method 2: Try esptool directly for more thorough cleaning
    print_color $YELLOW "Standard erase failed, trying esptool direct method..."
    
    # Find esptool in platformio
    local esptool_path=""
    if command -v esptool.py >/dev/null 2>&1; then
        esptool_path="esptool.py"
    elif [[ -f "$HOME/.platformio/packages/tool-esptoolpy/esptool.py" ]]; then
        esptool_path="python3 $HOME/.platformio/packages/tool-esptoolpy/esptool.py"
    fi
    
    if [[ -n "$esptool_path" ]]; then
        print_color $BLUE "Using esptool for thorough cleaning..."
        
        # Erase entire flash
        if $esptool_path --chip esp32s3 --port "$SELECTED_PORT" erase_flash; then
            print_color $GREEN "Complete flash erase with esptool successful"
            sleep 3
            return 0
        fi
        
        # If complete erase fails, try erasing specific regions
        print_color $YELLOW "Complete erase failed, trying selective region erase..."
        
        # Erase NVS partition (where network configs are stored)
        $esptool_path --chip esp32s3 --port "$SELECTED_PORT" erase_region 0x9000 0x5000 2>/dev/null
        # Erase PHY data partition
        $esptool_path --chip esp32s3 --port "$SELECTED_PORT" erase_region 0xe000 0x1000 2>/dev/null
        # Erase app partition
        $esptool_path --chip esp32s3 --port "$SELECTED_PORT" erase_region 0x10000 0x100000 2>/dev/null
        
        print_color $YELLOW "Selective region erase completed"
        sleep 2
        return 0
    fi
    
    # Method 3: Force clean build and upload
    print_color $YELLOW "Direct erase methods unavailable, using clean build approach..."
    if "$PIO_CMD" run -e "$BOARD_TYPE" --target clean; then
        print_color $BLUE "Build cache cleaned"
    fi
    
    print_color $YELLOW "Warning: Could not perform complete flash erase"
    print_color $YELLOW "Device may retain some previous configurations"
    print_color $YELLOW "New configuration should still override old settings"
    print_color $BLUE "This is not critical - proceeding with firmware flash"
    
    return 0  # Don't fail completely, just warn
}

# Function to flash firmware
flash_firmware() {
    print_color $YELLOW "\n[2/4] Flashing firmware..."
    
    print_color $BLUE "Debug: Script directory: $SCRIPT_DIR"
    print_color $BLUE "Debug: Project directory: $PROJECT_DIR"
    print_color $BLUE "Debug: PlatformIO command: $PIO_CMD"
    
    cd "$PROJECT_DIR" || {
        print_color $RED "ERROR: Cannot access project directory: $PROJECT_DIR"
        return 1
    }
    
    # Verify platformio.ini exists
    if [[ ! -f "platformio.ini" ]]; then
        print_color $RED "ERROR: platformio.ini not found in $PROJECT_DIR"
        print_color $YELLOW "Current directory: $(pwd)"
        print_color $YELLOW "Files in directory: $(ls -la)"
        return 1
    fi
    print_color $GREEN "Found platformio.ini in project directory"
    
    detect_board_type
    print_color $BLUE "Using board: $BOARD_TYPE"
    print_color $BLUE "Using environment: $BOARD_TYPE"
    print_color $BLUE "Using port: $SELECTED_PORT"
    print_color $BLUE "Project directory: $PROJECT_DIR"
    
    # First clean the device
    if ! clean_device_flash; then
        print_color $YELLOW "Warning: Device cleaning failed, proceeding anyway"
    fi
    
    # Small delay after erase
    sleep 2
    
    print_color $BLUE "Executing: $PIO_CMD run -e $BOARD_TYPE --target upload --upload-port $SELECTED_PORT"
    if "$PIO_CMD" run -e "$BOARD_TYPE" --target upload --upload-port "$SELECTED_PORT"; then
        print_color $GREEN "Firmware flashed successfully"
        return 0
    else
        print_color $RED "Firmware flash failed"
        print_color $YELLOW "Debug info:"
        print_color $YELLOW "- Current directory: $(pwd)"
        print_color $YELLOW "- PlatformIO command: $PIO_CMD"
        print_color $YELLOW "- Board type: $BOARD_TYPE"
        print_color $YELLOW "- Port: $SELECTED_PORT"
        print_color $YELLOW ""
        print_color $YELLOW "Manual solution:"
        print_color $YELLOW "1. Close any open serial monitors"
        print_color $YELLOW "2. Hold BOOT button"
        print_color $YELLOW "3. Press and release RESET button"
        print_color $YELLOW "4. Release BOOT button"
        print_color $YELLOW "5. Manual cleaning commands (run from $PROJECT_DIR):"
        print_color $YELLOW "   cd \"$PROJECT_DIR\""
        print_color $YELLOW "   $PIO_CMD run -e $BOARD_TYPE --target erase --upload-port $SELECTED_PORT"
        print_color $YELLOW "   $PIO_CMD run -e $BOARD_TYPE --target upload --upload-port $SELECTED_PORT"
        print_color $YELLOW "6. Or use esptool directly:"
        print_color $YELLOW "   esptool.py --chip esp32s3 --port $SELECTED_PORT erase_flash"
        return 1
    fi
}

# Function to configure device
configure_device() {
    print_color $YELLOW "\n[3/4] Configuring device..."
    
    # Wait for device reboot
    sleep 8
    
    # Re-detect port (may have changed after flash)
    if ! detect_ports; then
        print_color $RED "WARNING: Cannot detect port after flash"
        print_manual_config
        return 1
    fi
    
    print_color $BLUE "Using configuration port: $SELECTED_PORT"
    
    # Build configuration commands based on radio profile
    local channel_upper=$(echo "$CHANNEL" | tr '[:lower:]' '[:upper:]')
    local password_upper=$(echo "$PASSWORD" | tr '[:lower:]' '[:upper:]')
    local role_upper=$(echo "$ROLE" | tr '[:lower:]' '[:upper:]')
    local region_upper=$(echo "$REGION" | tr '[:lower:]' '[:upper:]')
    local mode_upper=$(echo "$MODE" | tr '[:lower:]' '[:upper:]')
    local radio_upper=$(echo "$RADIO" | tr '[:lower:]' '[:upper:]')
    
    local network_cmd="NETWORK_CREATE $channel_upper $password_upper"
    local config_cmd=""
    
    if [[ "$radio_upper" == "CUSTOM_ADVANCED" ]]; then
        config_cmd="Q_CONFIG $role_upper,$DEVICE_ID,$INTERVAL,$region_upper,$mode_upper,$radio_upper,$HOPS,$CUSTOM_BW,$CUSTOM_SF,$CUSTOM_CR,$CUSTOM_POWER,$CUSTOM_PREAMBLE"
    else
        config_cmd="Q_CONFIG $role_upper,$DEVICE_ID,$INTERVAL,$region_upper,$mode_upper,$radio_upper,$HOPS"
    fi
    
    local config_cmd_alt="CONFIG $role_upper,$DEVICE_ID,$INTERVAL,$region_upper,$mode_upper,$radio_upper,$HOPS"
    
    # Create Python script for serial communication
    local serial_script="
import serial
import time
import sys

try:
    ser = serial.Serial('$SELECTED_PORT', 115200, timeout=15)
    time.sleep(5)  # Wait for device initialization
    
    print('Clearing input buffer...')
    ser.reset_input_buffer()
    
    # Test communication
    print('Testing device communication...')
    ser.write(b'\r\n')
    time.sleep(1)
    ser.write(b'STATUS\r\n')
    time.sleep(2)
    
    # Clear status response
    while ser.in_waiting > 0:
        ser.read(ser.in_waiting)
        time.sleep(0.1)
    
    print('Device communication established')
    
    # Step 1: Create network
    print('Creating network: $channel_upper')
    ser.reset_input_buffer()
    ser.write(('$network_cmd' + '\r\n').encode())
    time.sleep(4)
    
    network_response = ''
    while ser.in_waiting > 0:
        network_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    print('Network response:', network_response[:200])
    
    # Step 2: Configure device
    print('Configuring device parameters...')
    ser.reset_input_buffer()
    ser.write(('$config_cmd' + '\r\n').encode())
    time.sleep(4)
    
    config_response = ''
    while ser.in_waiting > 0:
        config_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    print('Configuration response:', config_response[:200])
    
    # Try alternative if needed
    if ('comando desconocido' in config_response.lower() or 
        'unknown command' in config_response.lower() or
        len(config_response.strip()) < 10):
        
        print('Trying alternative CONFIG command...')
        ser.reset_input_buffer()
        ser.write(('$config_cmd_alt' + '\r\n').encode())
        time.sleep(4)
        
        while ser.in_waiting > 0:
            config_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
    
    # Step 3: Save configuration
    print('Saving configuration to EEPROM...')
    ser.reset_input_buffer()
    ser.write(b'CONFIG_SAVE\r\n')
    time.sleep(3)
    
    save_response = ''
    while ser.in_waiting > 0:
        save_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    print('Save response:', save_response[:100])
    
    # Step 4: Start device
    print('Starting device operation...')
    ser.reset_input_buffer()
    ser.write(b'START\r\n')
    time.sleep(2)
    
    # Final status
    ser.write(b'NETWORK_STATUS\r\n')
    time.sleep(2)
    
    final_response = ''
    while ser.in_waiting > 0:
        final_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    ser.close()
    
    # Check for success
    all_responses = (network_response + config_response + save_response + final_response).lower()
    success_indicators = [
        'network creada exitosamente',
        'configuracion guardada exitosamente',
        'configuración guardada',
        'network activa',
        'listo y operando'
    ]
    
    if any(indicator in all_responses for indicator in success_indicators):
        print('CONFIG_SUCCESS')
    else:
        print('CONFIG_UNCERTAIN')
        
except Exception as e:
    print('CONFIG_ERROR:', str(e))
"
    
    # Execute configuration
    local result=$(python3 -c "$serial_script" 2>&1)
    
    if [[ "$result" == *"CONFIG_SUCCESS"* ]]; then
        print_color $GREEN "Configuration completed successfully"
        return 0
    elif [[ "$result" == *"CONFIG_UNCERTAIN"* ]]; then
        print_color $YELLOW "Configuration may have failed"
        print_manual_verification
        return 0
    else
        print_color $RED "Configuration error"
        print_color $RED "$result"
        print_manual_config
        return 1
    fi
}

# Function to print manual configuration instructions
print_manual_config() {
    print_color $YELLOW "\n=== MANUAL RECOVERY ==="
    print_color $YELLOW "1. Connect manually: pio device monitor --port $SELECTED_PORT --baud 115200"
    print_color $YELLOW "2. Send these commands one by one:"
    print_color $YELLOW "   STATUS"
    local channel_upper=$(echo "$CHANNEL" | tr '[:lower:]' '[:upper:]')
    local password_upper=$(echo "$PASSWORD" | tr '[:lower:]' '[:upper:]')
    local role_upper=$(echo "$ROLE" | tr '[:lower:]' '[:upper:]')
    local region_upper=$(echo "$REGION" | tr '[:lower:]' '[:upper:]')
    local mode_upper=$(echo "$MODE" | tr '[:lower:]' '[:upper:]')
    local radio_upper=$(echo "$RADIO" | tr '[:lower:]' '[:upper:]')
    
    print_color $YELLOW "   NETWORK_CREATE $channel_upper $password_upper"
    if [[ "$radio_upper" == "CUSTOM_ADVANCED" ]]; then
        print_color $YELLOW "   Q_CONFIG $role_upper,$DEVICE_ID,$INTERVAL,$region_upper,$mode_upper,$radio_upper,$HOPS,$CUSTOM_BW,$CUSTOM_SF,$CUSTOM_CR,$CUSTOM_POWER,$CUSTOM_PREAMBLE"
    else
        print_color $YELLOW "   Q_CONFIG $role_upper,$DEVICE_ID,$INTERVAL,$region_upper,$mode_upper,$radio_upper,$HOPS"
    fi
    print_color $YELLOW "   CONFIG_SAVE"
    print_color $YELLOW "   START"
    print_color $YELLOW "   NETWORK_STATUS"
}

print_manual_verification() {
    print_color $YELLOW "Manual verification required:"
    print_color $YELLOW "1. Connect to device: pio device monitor --port $SELECTED_PORT --baud 115200"
    print_color $YELLOW "2. Check status: STATUS"
    print_color $YELLOW "3. Check network: NETWORK_STATUS"
}

# Function to launch serial monitor
launch_monitor() {
    print_color $YELLOW "\n[4/4] Launching serial monitor..."
    
    # Find the full path to pio command to ensure it works in new terminal
    local pio_path="$PIO_CMD"
    
    # If PIO_CMD is just 'pio', try to find full path
    if [[ "$PIO_CMD" == "pio" ]]; then
        pio_path=$(which pio 2>/dev/null)
        if [[ -z "$pio_path" ]]; then
            # Try common locations
            if [[ -f "$HOME/.local/bin/pio" ]]; then
                pio_path="$HOME/.local/bin/pio"
            elif [[ -f "$HOME/.platformio/penv/bin/pio" ]]; then
                pio_path="$HOME/.platformio/penv/bin/pio"
            else
                pio_path="$PIO_CMD"  # Use original as fallback
            fi
        fi
    fi
    
    # Create a temporary script file to avoid osascript parsing issues
    local temp_script="/tmp/custodia_monitor_$$.sh"
    cat > "$temp_script" << EOF
#!/bin/bash
cd '$PROJECT_DIR'
export PATH="\$PATH:$HOME/.local/bin:$HOME/.platformio/penv/bin"
echo "Custodia Device Monitor"
echo "======================"
echo "Port: $SELECTED_PORT"
echo "PlatformIO: $pio_path"
echo ""
'$pio_path' device monitor --baud 115200 --port $SELECTED_PORT
echo ""
echo "Monitor session ended. Press Enter to close..."
read
rm -f "$temp_script"
EOF
    chmod +x "$temp_script"
    
    # Launch monitor in new Terminal window using the temporary script
    if osascript -e "tell application \"Terminal\" to do script \"$temp_script\"" >/dev/null 2>&1; then
        print_color $GREEN "Monitor window opened successfully"
        print_color $BLUE "Check your dock for the new Terminal window"
    else
        print_color $YELLOW "Could not auto-launch monitor window"
        print_color $YELLOW "Manual command:"
        print_color $BLUE "$pio_path device monitor --baud 115200 --port $SELECTED_PORT"
        # Cleanup temp script if osascript failed
        rm -f "$temp_script"
    fi
    
    print_color $BLUE "Monitor command: $pio_path device monitor --baud 115200 --port $SELECTED_PORT"
}

# Function to show final summary
show_summary() {
    print_color $BLUE "\n===== CONFIGURATION SUMMARY ====="
    print_color $GREEN "Device ID: $DEVICE_ID"
    print_color $GREEN "Role: $ROLE"
    print_color $GREEN "Transmission Interval: $INTERVAL seconds"
    print_color $GREEN "Region: $REGION"
    print_color $GREEN "Mode: $MODE"
    print_color $GREEN "Radio Profile: $RADIO"
    if [[ "$RADIO" == "CUSTOM_ADVANCED" ]]; then
        print_color $GREEN "  Bandwidth: $CUSTOM_BW kHz"
        print_color $GREEN "  Spreading Factor: SF$CUSTOM_SF"
        print_color $GREEN "  Coding Rate: 4/$CUSTOM_CR"
        print_color $GREEN "  TX Power: $CUSTOM_POWER dBm"
        print_color $GREEN "  Preamble: $CUSTOM_PREAMBLE"
    fi
    print_color $GREEN "Max Hops: $HOPS"
    print_color $GREEN "Network Channel: $CHANNEL"
    print_color $GREEN "Network Password: $PASSWORD"
    print_color $GREEN "Network Hash: $(calculate_network_hash)"
    print_color $BLUE "=================================="
    echo
    print_color $BLUE "STATUS: READY FOR OPERATION"
    print_color $BLUE "Monitor launched in separate Terminal window"
    echo
}

# Main function
main() {
    # Check if running in Terminal
    if [[ ! -t 1 ]]; then
        # Not in terminal, launch in new Terminal window
        osascript -e "tell application \"Terminal\" to do script \"cd '$SCRIPT_DIR' && ./$(basename "$0") $*\""
        exit 0
    fi
    
    print_header
    
    # Check for help
    if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
        echo "Usage: $0 [interactive mode]"
        echo "       $0 -role ROLE -id ID -interval SECONDS -region REGION -mode MODE -radio PROFILE -channel NAME [OPTIONS]"
        echo
        echo "Interactive mode (no parameters): Guided configuration"
        echo "Command line mode: Direct parameter specification"
        echo
        echo "Examples:"
        echo "  $0  # Interactive mode"
        echo "  $0 -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA"
        exit 0
    fi
    
    # If no parameters provided, enter interactive mode
    if [[ $# -eq 0 ]]; then
        # Get configuration type
        if get_config_type; then
            # Normal interactive mode
            print_color $GREEN "Interactive Configuration Mode"
            get_role
            get_device_id
            get_interval
            get_region
            get_mode
            get_radio
            get_hops
            get_channel
            get_password
        else
            # One-line mode
            get_oneline_config
            
            # Validate parsed parameters
            if ! validate_all_parameters; then
                exit 1
            fi
        fi
    else
        # Command line mode - parse arguments
        while [[ $# -gt 0 ]]; do
            case $1 in
                -role)
                    ROLE="$2"
                    shift 2
                    ;;
                -id)
                    DEVICE_ID="$2"
                    shift 2
                    ;;
                -interval)
                    INTERVAL="$2"
                    shift 2
                    ;;
                -region)
                    REGION="$2"
                    shift 2
                    ;;
                -mode)
                    MODE="$2"
                    shift 2
                    ;;
                -radio)
                    RADIO="$2"
                    shift 2
                    ;;
                -hops)
                    HOPS="$2"
                    shift 2
                    ;;
                -channel)
                    CHANNEL="$2"
                    shift 2
                    ;;
                -password)
                    PASSWORD="$2"
                    shift 2
                    ;;
                -port)
                    SELECTED_PORT="$2"
                    shift 2
                    ;;
                *)
                    print_color $RED "Unknown parameter: $1"
                    exit 1
                    ;;
            esac
        done
        
        # Check required parameters for command line mode
        if [[ -z "$ROLE" ]] || [[ -z "$DEVICE_ID" ]] || [[ -z "$INTERVAL" ]] || [[ -z "$REGION" ]] || [[ -z "$MODE" ]] || [[ -z "$RADIO" ]] || [[ -z "$CHANNEL" ]]; then
            print_color $RED "Missing required parameters for command line mode"
            print_color $YELLOW "Required: -role -id -interval -region -mode -radio -channel"
            exit 1
        fi
        
        # Set defaults
        HOPS=${HOPS:-3}
        
        # Generate password if not provided
        if [[ -z "$PASSWORD" ]]; then
            generate_password
            print_color $GREEN "Password auto-generated: $PASSWORD"
        fi
        
        # Validate all parameters
        if ! validate_all_parameters; then
            exit 1
        fi
    fi
    
    print_color $GREEN "All parameters valid"
    
    # Install dependencies
    if ! install_dependencies; then
        print_color $RED "System setup failed"
        exit 1
    fi
    
    # Verify project structure first
    if ! verify_project_structure; then
        print_color $RED "Project structure verification failed"
        exit 1
    fi
    
    # Find or install PlatformIO
    print_color $YELLOW "Checking PlatformIO installation..."
    PIO_CMD=$(find_platformio)
    if [[ -z "$PIO_CMD" ]]; then
        if ! install_platformio; then
            exit 1
        fi
    else
        print_color $GREEN "PlatformIO found: $PIO_CMD"
    fi
    
    # Detect port if not provided
    if [[ -z "$SELECTED_PORT" ]]; then
        if ! detect_ports; then
            print_color $RED "Flash cancelled - No valid port selected"
            exit 1
        fi
    fi
    
    print_color $GREEN "Using port: $SELECTED_PORT"
    
    # Show configuration summary
    print_color $YELLOW "\n[CONFIG] Configuration Summary:"
    print_color $BLUE "Note: Device will be completely cleaned before flashing to ensure no previous configurations interfere"
    print_color $BLUE "Role: $ROLE"
    print_color $BLUE "ID: $DEVICE_ID"
    print_color $BLUE "Transmission interval: $INTERVAL seconds"
    print_color $BLUE "Region: $REGION"
    print_color $BLUE "Mode: $MODE"
    print_color $BLUE "Radio: $RADIO"
    if [[ "$RADIO" == "CUSTOM_ADVANCED" ]]; then
        print_color $BLUE "  Bandwidth: $CUSTOM_BW kHz"
        print_color $BLUE "  Spreading Factor: SF$CUSTOM_SF"
        print_color $BLUE "  Coding Rate: 4/$CUSTOM_CR"
        print_color $BLUE "  TX Power: $CUSTOM_POWER dBm"
        print_color $BLUE "  Preamble: $CUSTOM_PREAMBLE"
    fi
    print_color $BLUE "Hops: $HOPS"
    print_color $BLUE "Channel: $CHANNEL"
    print_color $BLUE "Password: $PASSWORD"
    print_color $BLUE "Network hash: $(calculate_network_hash)"
    echo
    
    # Confirm before proceeding
    read -p "Proceed with flashing? [Y/n]: " confirm
    if [[ "$confirm" =~ ^[Nn] ]]; then
        print_color $YELLOW "Operation cancelled by user"
        exit 0
    fi
    
    # Execute main workflow
    if flash_firmware && configure_device; then
        show_summary
        launch_monitor
    else
        print_color $RED "Flash tool completed with errors"
        exit 1
    fi
}

# Make sure we're in the right directory
cd "$SCRIPT_DIR" || {
    print_color $RED "ERROR: Cannot access script directory"
    exit 1
}

# Run main function
main "$@"