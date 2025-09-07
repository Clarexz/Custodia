#!/bin/bash

# Custodia Flash Tool Launcher
# Double-click this file to launch the flash tool in Terminal

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

clear
print_color $BLUE "=================================="
print_color $BLUE "    Custodia Flash Tool Launcher  "
print_color $BLUE "=================================="
echo

print_color $YELLOW "This launcher will help you configure and flash your ESP32 device."
echo

# Check if the main script exists
if [[ ! -f "$SCRIPT_DIR/Flash Tool MacOS/flash_tool_macos.sh" ]]; then
    print_color $RED "ERROR: flash_tool_macos.sh not found in Flash Tool MacOS directory"
    print_color $YELLOW "Please ensure the Flash Tool MacOS folder is present"
    read -p "Press Enter to exit..."
    exit 1
fi

# Function to validate device ID
validate_device_id() {
    local id="$1"
    if [[ "$id" =~ ^[0-9]+$ ]] && [ "$id" -ge 1 ] && [ "$id" -le 255 ]; then
        return 0
    fi
    return 1
}

# Function to validate interval
validate_interval() {
    local interval="$1"
    if [[ "$interval" =~ ^[0-9]+$ ]] && [ "$interval" -ge 5 ] && [ "$interval" -le 3600 ]; then
        return 0
    fi
    return 1
}

# Function to validate hops
validate_hops() {
    local hops="$1"
    if [[ "$hops" =~ ^[0-9]+$ ]] && [ "$hops" -ge 1 ] && [ "$hops" -le 10 ]; then
        return 0
    fi
    return 1
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
    
    # Check if same as channel (this will be checked later when we have both)
    return 0
}

# Configuration type selection
while true; do
    echo
    print_color $BLUE "Configuration Type:"
    echo "   1. Normal     - Interactive step-by-step configuration"
    echo "   2. One Line   - Single command line configuration"
    echo
    read -p "Select configuration type [1-2]: " config_choice
    
    case $config_choice in
        1) 
            CONFIG_TYPE="normal"
            break 
            ;;
        2) 
            CONFIG_TYPE="oneline"
            break 
            ;;
        *) 
            print_color $RED "Invalid selection. Please enter 1 or 2" 
            ;;
    esac
done

if [[ "$CONFIG_TYPE" == "oneline" ]]; then
    # One-line configuration mode
    print_color $BLUE "One-line Configuration Mode"
    echo
    print_color $YELLOW "Simply enter the parameters - the script path will be added automatically!"
    echo
    print_color $YELLOW "Example:"
    print_color $GREEN "-role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3"
    echo
    print_color $YELLOW "Available parameters:"
    print_color $BLUE "  -role [TRACKER|RECEIVER|REPEATER]"
    print_color $BLUE "  -id [1-255]"
    print_color $BLUE "  -interval [5-3600]"
    print_color $BLUE "  -region [US|EU|CH|AS|JP]"
    print_color $BLUE "  -mode [SIMPLE|ADMIN]"
    print_color $BLUE "  -radio [DESERT_LONG_FAST|MOUNTAIN_STABLE|URBAN_DENSE|MESH_MAX_NODES|CUSTOM_ADVANCED]"
    print_color $BLUE "  -channel [channel_name]"
    print_color $BLUE "  -password [password] (optional)"
    print_color $BLUE "  -hops [1-10] (optional, default: 3)"
    echo
    
    read -p "Enter your configuration parameters: " config_params
    
    # Build the complete command with script path and parameters
    FLASH_SCRIPT="\"$SCRIPT_DIR/Flash Tool MacOS/flash_tool_macos.sh\""
    COMPLETE_COMMAND="$FLASH_SCRIPT $config_params"
    
    # Execute the complete command
    print_color $YELLOW "Executing configuration..."
    print_color $BLUE "Running: $COMPLETE_COMMAND"
    echo
    bash -c "$COMPLETE_COMMAND"
    exit $?
fi

# Interactive parameter collection (normal mode)
print_color $GREEN "Please provide the following configuration parameters:"
echo

# Device Role
while true; do
    print_color $BLUE "1. Device Role:"
    echo "   1. TRACKER  - GPS tracking device"
    echo "   2. RECEIVER - Base station/receiver"
    echo "   3. REPEATER - Signal repeater/relay"
    echo
    read -p "Select role [1-3]: " choice
    
    case $choice in
        1) ROLE="TRACKER"; break ;;
        2) ROLE="RECEIVER"; break ;;
        3) ROLE="REPEATER"; break ;;
        *) print_color $RED "Invalid selection. Please enter 1, 2, or 3" ;;
    esac
done

# Device ID
while true; do
    print_color $BLUE "2. Device ID:"
    echo "   Unique identifier for this device (1-255)"
    echo
    read -p "Enter device ID [1-255]: " DEVICE_ID
    
    if validate_device_id "$DEVICE_ID"; then
        break
    else
        print_color $RED "Invalid device ID. Please enter a number between 1 and 255"
    fi
done

# Transmission Interval
while true; do
    print_color $BLUE "3. Transmission Interval:"
    echo "   How often device sends GPS updates (in seconds)"
    echo "   Examples: 30 (every 30 sec), 60 (every minute), 300 (every 5 min)"
    echo
    read -p "Enter interval in seconds [5-3600]: " INTERVAL
    
    if validate_interval "$INTERVAL"; then
        break
    else
        print_color $RED "Invalid interval. Please enter a number between 5 and 3600 seconds"
    fi
done

# Region
while true; do
    print_color $BLUE "4. LoRa Region:"
    echo "   1. US - United States"
    echo "   2. EU - Europe"
    echo "   3. CH - China"
    echo "   4. AS - Asia"
    echo "   5. JP - Japan"
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

# Mode
while true; do
    print_color $BLUE "5. Operation Mode:"
    echo "   1. SIMPLE - Basic operation"
    echo "   2. ADMIN  - Advanced features enabled"
    echo
    read -p "Select mode [1-2]: " choice
    
    case $choice in
        1) MODE="SIMPLE"; break ;;
        2) MODE="ADMIN"; break ;;
        *) print_color $RED "Invalid selection. Please enter 1 or 2" ;;
    esac
done

# Radio Profile
while true; do
    print_color $BLUE "6. Radio Profile:"
    echo "   1. DESERT_LONG_FAST  - Long range, fast transmission"
    echo "   2. MOUNTAIN_STABLE   - Stable connection in mountains"
    echo "   3. URBAN_DENSE       - Dense urban environments"
    echo "   4. MESH_MAX_NODES    - Maximum mesh network nodes"
    echo "   5. CUSTOM_ADVANCED   - Custom advanced settings"
    echo
    read -p "Select radio profile [1-5]: " choice
    
    case $choice in
        1) RADIO="DESERT_LONG_FAST"; break ;;
        2) RADIO="MOUNTAIN_STABLE"; break ;;
        3) RADIO="URBAN_DENSE"; break ;;
        4) RADIO="MESH_MAX_NODES"; break ;;
        5) RADIO="CUSTOM_ADVANCED"; 
           print_color $YELLOW "Note: Custom advanced parameters will be configured in the main tool"
           break ;;
        *) print_color $RED "Invalid selection. Please enter 1-5" ;;
    esac
done

# Optional Parameters  
print_color $BLUE "7. Optional Parameters:"

# Hops
while true; do
    print_color $BLUE "Max Hops:"
    echo "   Maximum number of hops for mesh network (1-10)"
    echo "   Default: 3"
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

# Channel Name
while true; do
    print_color $BLUE "8. Network Channel:"
    echo "   Network name for your devices (3-20 characters)"
    echo "   Only letters, numbers, and underscore allowed"
    echo "   Examples: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS"
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

# Password
while true; do
    print_color $BLUE "9. Network Password:"
    echo "   Password requirements:"
    echo "   - 8-32 characters"
    echo "   - Must contain at least 1 number and 1 letter"
    echo "   - Cannot be the same as channel name"
    echo "   Examples: Secure123, MyPass99, Field2024"
    echo
    read -p "Enter password or press Enter for auto-generation: " PASSWORD
    
    if [[ -z "$PASSWORD" ]]; then
        print_color $GREEN "Password will be auto-generated"
        break
    elif validate_password "$PASSWORD"; then
        # Check if password is same as channel
        password_upper=$(echo "$PASSWORD" | tr '[:lower:]' '[:upper:]')
        if [[ "$password_upper" == "$CHANNEL" ]]; then
            print_color $RED "Password cannot be the same as channel name"
        else
            PASSWORD=$(echo "$PASSWORD" | tr '[:lower:]' '[:upper:]')
            break
        fi
    else
        print_color $RED "Invalid password. Requirements:"
        print_color $RED "  - 8-32 characters"
        print_color $RED "  - At least 1 number and 1 letter"
        print_color $RED "  - Cannot be same as channel name"
    fi
done

echo
print_color $YELLOW "Configuration Summary:"
print_color $GREEN "Role: $ROLE"
print_color $GREEN "ID: $DEVICE_ID"
print_color $GREEN "Interval: $INTERVAL seconds"
print_color $GREEN "Region: $REGION"
print_color $GREEN "Mode: $MODE"
print_color $GREEN "Radio: $RADIO"
print_color $GREEN "Hops: $HOPS"
print_color $GREEN "Channel: $CHANNEL"
if [[ -n "$PASSWORD" ]]; then
    print_color $GREEN "Password: $PASSWORD"
else
    print_color $GREEN "Password: [will be auto-generated]"
fi

echo
read -p "Proceed with these settings? [Y/n]: " confirm
if [[ "$confirm" =~ ^[Nn] ]]; then
    print_color $YELLOW "Configuration cancelled"
    read -p "Press Enter to exit..."
    exit 0
fi

# Build command
CMD="\"$SCRIPT_DIR/Flash Tool MacOS/flash_tool_macos.sh\""
CMD="$CMD -role '$ROLE'"
CMD="$CMD -id '$DEVICE_ID'"
CMD="$CMD -interval '$INTERVAL'"
CMD="$CMD -region '$REGION'"
CMD="$CMD -mode '$MODE'"
CMD="$CMD -radio '$RADIO'"
CMD="$CMD -hops '$HOPS'"
CMD="$CMD -channel '$CHANNEL'"

if [[ -n "$PASSWORD" ]]; then
    CMD="$CMD -password '$PASSWORD'"
fi

print_color $YELLOW "Launching flash tool..."
echo

# Execute the main script
eval "$CMD"

# Keep terminal open
echo
print_color $BLUE "Flash tool execution completed."
read -p "Press Enter to close this window..."