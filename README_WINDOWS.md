# Custodia Flash Tool - Windows Guide

## Overview

The Custodia Flash Tool for Windows is a batch script that automatically flashes and configures ESP32 devices for the Custodia mesh network system. This tool is equivalent to the macOS version but designed specifically for Windows systems.

## Prerequisites

### Required Software
1. **Python 3.x** - Download from [python.org](https://python.org)
   - Make sure to check "Add Python to PATH" during installation
2. **PlatformIO Core** - Will be installed automatically by the script
3. **USB drivers for ESP32** - Usually installed automatically by Windows

### Hardware Requirements
- ESP32 development board (tested with Seeed XIAO ESP32S3)
- USB cable for connecting ESP32 to computer
- Windows 10 or Windows 11

## Installation

1. Ensure Python is installed and added to PATH
2. Download the complete Custodia project folder
3. Locate the `flash_tool_windows.bat` file in the main project directory
4. The tool will automatically install required dependencies on first run

## Usage

### Method 1: Double-Click (Interactive Mode)

1. Double-click `flash_tool_windows.bat`
2. Follow the step-by-step prompts to configure your device
3. The tool will guide you through:
   - Device role selection (TRACKER/RECEIVER/REPEATER)
   - Device ID assignment (1-255)
   - Transmission interval settings
   - LoRa region selection
   - Operation mode (SIMPLE/ADMIN)
   - Radio profile selection
   - Network configuration (channel name and password)

### Method 2: Command Line (One-Line Mode)

Open Command Prompt and run:

```cmd
flash_tool_windows.bat -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3
```

#### Available Parameters

- `-role` : Device role [TRACKER|RECEIVER|REPEATER]
- `-id` : Device ID [1-255]
- `-interval` : Transmission interval in seconds [5-3600]
- `-region` : LoRa region [US|EU|CH|AS|JP]
- `-mode` : Operation mode [SIMPLE|ADMIN]
- `-radio` : Radio profile [DESERT_LONG_FAST|MOUNTAIN_STABLE|URBAN_DENSE|MESH_MAX_NODES|CUSTOM_ADVANCED]
- `-channel` : Network channel name (3-20 characters, alphanumeric + underscore)
- `-password` : Network password (8-32 characters, optional - auto-generated if omitted)
- `-hops` : Maximum mesh hops [1-10] (optional, default: 3)

#### Example Configurations

**GPS Tracker:**
```cmd
flash_tool_windows.bat -role TRACKER -id 5 -interval 30 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel FIELD_OPS -password Track2024
```

**Base Station:**
```cmd
flash_tool_windows.bat -role RECEIVER -id 1 -interval 120 -region EU -mode ADMIN -radio MOUNTAIN_STABLE -channel BASE_ALPHA -password Base123
```

**Repeater Node:**
```cmd
flash_tool_windows.bat -role REPEATER -id 10 -interval 300 -region US -mode SIMPLE -radio MESH_MAX_NODES -channel MESH_NET -hops 5
```

## Radio Profiles

### Standard Profiles
- **DESERT_LONG_FAST**: Optimized for long-range communication with fast data rates
- **MOUNTAIN_STABLE**: Enhanced stability for mountainous terrain
- **URBAN_DENSE**: Configured for dense urban environments with interference
- **MESH_MAX_NODES**: Supports maximum number of mesh network nodes

### Custom Advanced Profile
When selecting CUSTOM_ADVANCED, you'll configure:
- **Bandwidth**: 7.8kHz to 500kHz
- **Spreading Factor**: 6 (fast/short) to 12 (slow/long range)
- **Coding Rate**: 4/5 (fast) to 4/8 (robust)
- **TX Power**: 2-22 dBm
- **Preamble Length**: 6-65535

## Troubleshooting

### Common Issues

#### "Python not found"
- Install Python from python.org
- Ensure "Add Python to PATH" was checked during installation
- Restart Command Prompt after installation

#### "No ESP32 devices detected"
- Check USB cable connection
- Try a different USB port
- Install ESP32 USB drivers manually if needed
- Ensure no other programs are using the serial port

#### "PlatformIO installation failed"
- Run Command Prompt as Administrator
- Manually install: `pip install platformio`
- Restart Command Prompt after installation

#### "Flash failed"
Solutions to try:
1. Put ESP32 in bootloader mode:
   - Hold BOOT button
   - Press and release RESET button
   - Release BOOT button
   - Try flashing again

2. Close any serial monitor programs (Arduino IDE, PuTTY, etc.)

3. Try a different USB cable or port

#### "Configuration failed"
- Ensure device flashed successfully first
- Wait 10 seconds after flashing before configuration
- Try manual configuration using the provided commands

### Manual Recovery

If automatic configuration fails, you can configure manually:

1. Open Command Prompt in project directory
2. Run: `pio device monitor --port COMX --baud 115200` (replace COMX with your port)
3. Send these commands one by one:
   ```
   STATUS
   NETWORK_CREATE YOUR_CHANNEL YOUR_PASSWORD
   Q_CONFIG ROLE,ID,INTERVAL,REGION,MODE,RADIO,HOPS
   CONFIG_SAVE
   START
   NETWORK_STATUS
   ```

## Network Configuration Guidelines

### Channel Names
- 3-20 characters
- Only letters, numbers, and underscores
- Cannot use reserved names: CONFIG, ADMIN, DEBUG, SYSTEM, DEVICE, LORA, MESH, TEST, DEFAULT
- Examples: `TEAM_ALPHA`, `BASE_01`, `MOUNTAIN_OPS`

### Passwords
- 8-32 characters
- Must contain at least 1 number and 1 letter
- Cannot be the same as channel name
- Auto-generated if not provided
- Examples: `Secure123`, `MyPass99`, `Field2024`

## File Structure

The tool expects this project structure:
```
Custodia/
├── flash_tool_windows.bat    <- Main Windows tool
├── platformio.ini           <- PlatformIO configuration
├── src/                     <- Source code directory
└── README_WINDOWS.md        <- This file
```

## Support

### Device Monitoring
After flashing, the tool will open a serial monitor window to view device status and debug information.

### Log Files
Check Windows Event Viewer for system-level USB and driver issues if devices aren't detected.

### Performance Tips
- Close unnecessary programs before flashing
- Use high-quality USB cables
- Avoid USB hubs when possible
- Keep ESP32 close to computer during configuration

## Version Information

- **Windows Batch Version**: 2.0
- **Compatible ESP32 Boards**: Seeed XIAO ESP32S3, ESP32 DevKit, ESP32-WROOM
- **Supported Windows**: Windows 10, Windows 11
- **Python Requirement**: 3.7 or higher
- **PlatformIO**: Latest version (auto-installed)

## Security Notes

- Network passwords are transmitted over USB serial connection only
- Passwords are stored in device flash memory
- Use strong passwords for production deployments
- Consider changing default passwords for security-critical applications