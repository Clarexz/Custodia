# Custom LoRa Mesh Asset Tracking System

**Complete IoT mesh system with encrypted networks and automated deployment tools**

![Project Status](https://img.shields.io/badge/Status-Production%20Ready-brightgreen)
![Version](https://img.shields.io/badge/Version-1.0.0-blue)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Tools](https://img.shields.io/badge/Tools-Cross--Platform-green)

---

## Project Description

Complete off-grid GPS tracking system based on Meshtastic algorithm with simplified encrypted networks implementation. The system features automated cross-platform deployment tools, unified hardware management, and secure mesh communications with network isolation capabilities.

### Main Features

- **Complete Meshtastic algorithm** implementation with mesh routing
- **Cross-platform build workflow** using PlatformIO
- **Encrypted private networks** with automatic isolation
- **Unified GPIO template** for all peripheral management
- **Optimized radio profiles** for different environments
- **Remote device configuration** capabilities
- **Multi-region LoRa support** (US/EU/CH/AS/JP)

---

## System Architecture

### Device Roles

#### TRACKER
- Transmits real GPS position periodically
- Battery monitoring with automatic voltage reporting
- Participates in mesh routing with CLIENT priority
- Configurable via remote commands

#### REPEATER
- Extends network range via intelligent packet retransmission
- SNR-based delay optimization for collision avoidance
- ROUTER priority for efficient mesh routing
- Remote configuration support

#### RECEIVER
- Centralized network control and monitoring
- Mass device configuration capabilities
- Network discovery and management
- Complete mesh statistics and diagnostics

---

## Build & Deployment

### Prerequisites
- Python 3.x installed
- PlatformIO Core (`pip install platformio`)
- USB cable for the target board (ESP32-S3 or XIAO nRF52840)

### 1. Clone & Install Dependencies
```bash
git clone https://github.com/Clarexz/meshtastic_custom.git
cd Custodia
pip install -r requirements.txt
```

### 2. Build & Flash Firmware
Select the environment that matches your board:

```bash
# ESP32-S3 (Seeed XIAO ESP32S3)
python3 -m platformio run -e seeed_xiao_esp32s3 -t upload

# nRF52840 (Seeed XIAO nRF52840)
python3 -m platformio run -e seeed_xiao_nrf52840 -t upload
```

### 3. Open Serial Console
After flashing, open a serial monitor at 115200 baud to access the configuration shell:

```bash
python3 -m platformio device monitor --baud 115200 --port <your_port>
```

When the device boots it waits ~5 seconds for input before entering `STATE_RUNNING`. Press any key during that window or run `CONFIG` later to enter configuration mode. Commands are case-insensitive but are echoed in uppercase for clarity.

### 4. Manual Configuration Commands
Use the following commands from the serial prompt to configure each device. Finish with `CONFIG_SAVE` to persist the settings and `EXIT` (or reset) to start operation.

**Common Commands**
```
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>
CONFIG_DEVICE_ID <1-999>
CONFIG_GPS_INTERVAL <seconds>
CONFIG_MAX_HOPS <1-10>
CONFIG_DATA_MODE <SIMPLE|ADMIN>
CONFIG_REGION <US|EU|CH|AS|JP>
CONFIG_RADIO_PROFILE <PROFILE_NAME>
MODE <SIMPLE|ADMIN>
CONFIG_SAVE
STATUS
INFO
HELP
```

**Example: Tracker**
```
CONFIG_ROLE TRACKER
CONFIG_DEVICE_ID 1
CONFIG_GPS_INTERVAL 15
CONFIG_MAX_HOPS 3
CONFIG_DATA_MODE SIMPLE
CONFIG_REGION US
CONFIG_RADIO_PROFILE DESERT_LONG_FAST
CONFIG_SAVE
```

Tip: use `Q_CONFIG ROLE,ID,GPS_INTERVAL,REGION,DATA_MODE,RADIO_PROFILE[,MAX_HOPS]` to set the most common fields in a single command (e.g. `Q_CONFIG TRACKER,001,15,US,SIMPLE,DESERT_LONG_FAST,3`).

**Example: Repeater**
```
CONFIG_ROLE REPEATER
CONFIG_DEVICE_ID 200
CONFIG_DATA_MODE ADMIN
CONFIG_REGION US
CONFIG_RADIO_PROFILE MESH_MAX_NODES
CONFIG_SAVE
```

**Example: Receiver**
```
CONFIG_ROLE RECEIVER
CONFIG_DEVICE_ID 500
CONFIG_DATA_MODE ADMIN
CONFIG_REGION US
CONFIG_RADIO_PROFILE URBAN_DENSE
CONFIG_SAVE
```

After saving, the device reboots into operational mode using the stored configuration. To change settings later, enter `CONFIG` to re-open the shell or issue remote commands from a receiver node.

### Network Security
Create isolated networks by choosing distinct channel/password pairs via the configuration shell. Use commands such as `NETWORK_CREATE <name> [password]`, `NETWORK_JOIN <name> <password>`, `NETWORK_LIST`, and `NETWORK_DELETE <name>` to manage stored networks. Passwords must be 8-32 characters with letters and numbers; the firmware enforces validation during entry.

---

## Hardware Configuration

### Unified GPIO Template

The system uses a centralized GPIO management approach with all pin assignments unified in `user_logic.h`:

**LoRa SX1262 Transceiver:**
```cpp
#define LORA_SCK_PIN 7      // SPI Clock
#define LORA_MISO_PIN 8     // SPI MISO
#define LORA_MOSI_PIN 9     // SPI MOSI
#define LORA_CS_PIN 41      // Chip Select
#define LORA_DIO1_PIN 39    // Interrupt
#define LORA_RST_PIN 42     // Reset
#define LORA_BUSY_PIN 40    // Busy Signal
```

**System Control:**
```cpp
#define LED_PIN 21              // Status LED
#define BATTERY_ADC_PIN 0       // Battery voltage monitor
#define UART_DEBUG_PIN 43/44    // Serial debug interface
```

**Available for User Applications (10 pins):**
- **Pin 1** (ADC capable for analog sensors)
- **Pins 10-18** (Digital GPIO)
- **Total ADC channels available**: 2 (pins 1, 10)

### Hardware Integration Example

```cpp
#include "user_logic.h"

void setup() {
    // System peripherals automatically configured
    
    // User application example
    int sensorValue = analogRead(1);        // Pin 1 (ADC)
    digitalWrite(10, HIGH);                 // Pin 10 (Digital)
    
    // All system pins are protected and pre-configured
}
```

---

## Radio Profiles

### Available Optimization Profiles

The tool now bundles the original Custodia presets plus the most common Meshtastic radio profiles. Select whichever best matches your deployment goals:

| Profile | SF | BW (kHz) | CR | Power (dBm) | Typical Range | Airtime (44B) | Notes |
|---------|----|----------|----|-------------|---------------|---------------|-------|
| **SHORT_TURBO** | 7 | 500 | 4/5 | 20 | ~0.6 km | ~40 ms | Ultra-fast lab / experimental profile (500 kHz may be illegal in some regions) |
| **SHORT_FAST** | 7 | 250 | 4/5 | 20 | ~0.9 km | ~60 ms | High throughput for dense urban meshes |
| **SHORT_SLOW** | 8 | 250 | 4/5 | 20 | ~1.2 km | ~110 ms | Balanced short-to-medium range coverage |
| **MEDIUM_FAST** | 9 | 250 | 4/5 | 20 | ~1.8 km | ~180 ms | Great default for growing suburban networks |
| **MEDIUM_SLOW** | 10 | 250 | 4/5 | 20 | ~2.2 km | ~260 ms | Extra reach without huge airtime penalties |
| **LONG_FAST** | 11 | 250 | 4/8 | 20 | ~2.6 km | ~400 ms | Meshtastic default profile |
| **LONG_MODERATE** | 11 | 125 | 4/6 | 20 | ~3.2 km | ~650 ms | Extended reach with moderate airtime |
| **LONG_SLOW** | 12 | 125 | 4/8 | 20 | ~4.5 km | ~1.1 s | Maximum reach and sensitivity |
| **DESERT_LONG_FAST** | 12 | 125 | 4/5 | 20 | ~8 km | ~2.2 s | Legacy Custodia long-haul (open terrain) |
| **MOUNTAIN_STABLE** | 10 | 125 | 4/6 | 17 | ~4 km | ~0.9 s | Legacy rugged / obstacle-heavy deployments |
| **URBAN_DENSE** | 7 | 250 | 4/5 | 10 | ~0.8 km | ~80 ms | Legacy high-density testing profile |
| **MESH_MAX_NODES** | 8 | 125 | 4/5 | 14 | ~2.5 km | ~320 ms | Legacy mesh balance (20-30 nodes) |
| **CUSTOM_ADVANCED** | Var | Var | Var | Var | Depends | Depends | Manual configuration via `RADIO_PROFILE_CUSTOM` |

Detailed engineering notes for each profile live in [`meshtastic_radio_profiles.md`](meshtastic_radio_profiles.md).

### Profile Configuration Examples

**Deployment with specific profile (serial shell):**
```
CONFIG_ROLE TRACKER
CONFIG_DEVICE_ID 1
CONFIG_RADIO_PROFILE DESERT_LONG_FAST
CONFIG_SAVE

CONFIG_ROLE REPEATER
CONFIG_DEVICE_ID 2
CONFIG_RADIO_PROFILE URBAN_DENSE
CONFIG_SAVE

CONFIG_ROLE RECEIVER
CONFIG_DEVICE_ID 3
CONFIG_RADIO_PROFILE MESH_MAX_NODES
CONFIG_SAVE
```

**Manual profile customization (via serial interface):**
```bash
CONFIG_RADIO_PROFILE CUSTOM_ADVANCED
RADIO_PROFILE_CUSTOM SF 8
RADIO_PROFILE_CUSTOM BW 125
RADIO_PROFILE_CUSTOM POWER 17
RADIO_PROFILE_APPLY
CONFIG_SAVE
```

---

## Network Security

### Private Encrypted Networks

The system implements simplified network security with automatic encryption and device isolation:

**Security Features:**
- **Automatic AES encryption** for all network communications
- **Complete network isolation** - devices on different networks cannot communicate
- **Persistent network configuration** survives device reboots
- **Simple network management** via intuitive commands

### Network Management Commands

**Available via serial interface after deployment:**

```bash
NETWORK_LIST                            # Show all configured networks
NETWORK_CREATE <name> [password]        # Create new encrypted network
NETWORK_JOIN <name> <password>          # Join existing network
NETWORK_INFO [name]                     # Detailed network information
NETWORK_STATUS                          # Current network status
NETWORK_DELETE <name>                   # Delete network (requires confirmation)
```

### Network Security Examples

**Create isolated network teams:**
```bash
# Team A network
NETWORK_CREATE TEAM_ALPHA
[OK] Network 'TEAM_ALPHA' created successfully.
[INFO] Password auto-generated: DG13CJP6

# Team B network (completely isolated)
NETWORK_CREATE TEAM_BETA SECRET123
[OK] Network 'TEAM_BETA' created successfully.
```

**Network isolation demonstration:**
- Devices on "TEAM_ALPHA" network can only communicate with other "TEAM_ALPHA" devices
- Devices on "TEAM_BETA" network form completely separate mesh
- Packets are automatically encrypted and filtered by network hash
- No configuration required beyond network selection

## GPS Logic Integration

- Toda la lógica del GPS vive en `src/gps/gps_logic.cpp`. El archivo incluye la simulación por defecto, y al final encontrarás las variables `gpsData` y `gpsStatus`, además de funciones públicas que el resto del firmware consume.
- Para conectar un módulo real solo tienes que reemplazar el contenido de ese archivo por tu implementación, manteniendo las variables y firmas expuestas. El resto del proyecto seguirá funcionando sin cambios.
- `gps_logic.h` declara el “contrato” (funciones `gpsLogicBegin`, `gpsLogicUpdate`, etc.). Si tu código no soporta ciertas opciones (p.ej. simulación), deja las funciones como no-op y el sistema mostrará mensajes informativos.
- `GPSManager` se limita a leer esas variables, formatear paquetes y ofrecer utilidades; no necesitas modificarlo al cambiar de hardware.

---

## LoRa Regional Support

| Region | Code | Frequency | Coverage Areas |
|--------|------|-----------|----------------|
| **US** | `US` | 915 MHz | United States, Mexico, Canada |
| **EU** | `EU` | 868 MHz | Europe, Africa, Middle East |
| **CH** | `CH` | 470 MHz | China |
| **AS** | `AS` | 433 MHz | Asia (general) |
| **JP** | `JP` | 920 MHz | Japan |

Configure each device with `CONFIG_REGION <code>` to select the appropriate regulatory plan before saving the configuration.

---

## Data Visualization

### SIMPLE Mode (Clean Output)

**Purpose**: Minimal display showing only essential packet data

**Output format:**
```
[001,25.302677,-98.277664,4100,1718661234]
Transmission completed
```

### ADMIN Mode (Detailed Diagnostics)

**Purpose**: Complete system information with mesh statistics

**Output format:**
```
[TRACKER] === GPS + LoRa MESH TRANSMISSION ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Region: US (915.0 MHz)
Radio Profile: DESERT_LONG_FAST
Network: TEAM_ALPHA (ENCRYPTED)
Coordinates: 25.302677,-98.277664
Battery: 4100 mV
Timestamp: 1718661234
Packet: [001,25.302677,-98.277664,4100,1718661234]
LoRa Status: SENT
Last RSSI: -65.5 dBm
Last SNR: 10.2 dB
Packets sent: 15
Duplicates ignored: 0
Retransmissions: 3
Network packets filtered: 2
Next transmission in 15 seconds
==========================================
```

---

## Packet Format and Protocol

### GPS Data Structure

**Standard packet format:**
```
[deviceID, latitude, longitude, batteryvoltage, timestamp]
```

**Example:**
```
[001,25.302677,-98.277664,4100,1718661234]
```

### Packet Field Definitions

| Field | Description | Format | Range | Example |
|-------|-------------|--------|-------|---------|
| `deviceID` | Unique device identifier | 001-999 | 1-999 | `001` |
| `latitude` | Latitude in decimal degrees | 6 decimals | ±90.0 | `25.302677` |
| `longitude` | Longitude in decimal degrees | 6 decimals | ±180.0 | `-98.277664` |
| `batteryvoltage` | Battery voltage in millivolts | Integer | 3000-4200 | `4100` |
| `timestamp` | Unix timestamp | Seconds | Current time | `1718661234` |

### Network Protocol Features

**Packet Processing:**
- **Binary packet structure** for efficient transmission (44 bytes total)
- **Network hash inclusion** for automatic filtering
- **16-bit XOR checksum** for integrity verification
- **Hop count management** with maximum 3 hops
- **Duplicate detection** with 100-packet memory

**Mesh Routing Algorithm:**
- Complete **Meshtastic flood routing** implementation
- **SNR-based transmission delays** (distant nodes transmit first)
- **Role-based priority** (REPEATER = 160ms less delay than TRACKER)
- **Intelligent collision avoidance** via contention window

---

## System Performance

### Network Capacity

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Maximum theoretical nodes** | 100-150 | Based on duplicate detection table |
| **Recommended operational nodes** | 20-30 | Optimal performance |
| **Maximum hops** | 3 | Configurable via deployment |
| **Packet memory** | 100 packets | Duplicate detection buffer |
| **Network isolation** | Complete | Different networks cannot communicate |

### Operating Parameters

| Scenario | GPS Interval | Network Load | Performance |
|----------|-------------|--------------|-------------|
| **Small network (5 nodes)** | 15s | Low | Excellent |
| **Medium network (10 nodes)** | 30s | Medium | Good |
| **Large network (20 nodes)** | 60s | High | Acceptable |
| **Maximum network (30+ nodes)** | 120s+ | Critical | Degraded |

---

## Troubleshooting

### Flash Tool Issues

**ESP32-S3 not detected:**
```bash
# Solutions:
1. Verify USB-C connection integrity
2. Temporarily disconnect LoRa module (potential interference)
3. Use manual BOOT+RESET sequence
4. Install/update USB drivers
5. Try different USB port or cable
6. Use -port parameter for manual override
```

**Integrated monitor fails to launch:**
```bash
# Fallback manual monitor:
pio device monitor --baud 115200

# Platform-specific solutions:
# Windows: Ensure cmd.exe available in PATH
# Mac: Verify Terminal.app permissions
# Linux: Install gnome-terminal, xterm, or konsole
```

**Password validation errors:**
```bash
# Common issues and solutions:
1. "Password insecure" - Ensure at least 1 number AND 1 letter
2. "Password too short" - Minimum 8 characters required
3. "Password cannot be same as channel" - Use different password
4. "Reserved name" - Avoid CONFIG, ADMIN, DEBUG, SYSTEM, etc.
```

**Configuration persistence issues:**
```bash
# Verification steps:
1. Confirm CONFIG_SAVE was successful
2. Check NETWORK_LIST shows created network
3. Verify device restarts with same network active
4. Use STATUS command to check configuration
```

### Network Communication Issues

**Devices not communicating:**
```bash
# Verification steps:
1. Confirm all devices use same region (US, EU, etc.)
2. Verify all devices on same network channel
3. Check devices are within LoRa range
4. Verify compatible radio profiles
```

**Network security problems:**
```bash
# Debug steps:
1. Check active network: NETWORK_STATUS
2. List available networks: NETWORK_LIST
3. Verify network passwords match
4. Confirm network creation was successful
5. Check for typos in network names (case-sensitive)
```

### Hardware Configuration Issues

**GPIO conflicts:**
```bash
# Resolution:
1. Review user_logic.h for available pins
2. Verify no conflicts with system peripherals
3. Use only pins 1, 10-18 for user applications
4. Check ADC pin limitations (only pins 1, 10)
```

**LoRa module issues:**
```bash
# Diagnostics:
1. Verify SX1262 wiring per GPIO template
2. Check 3.3V power supply stability
3. Confirm all SPI connections
4. Test with known working radio profile
5. Check for hardware damage or loose connections
```

---

## Manual Configuration Interface

For advanced users or troubleshooting, devices provide a complete serial command interface:

### Basic Configuration Commands

```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>     # Set device role
CONFIG_DEVICE_ID <1-999>                    # Set unique identifier  
CONFIG_GPS_INTERVAL <5-3600>                # GPS transmission interval
CONFIG_REGION <US|EU|CH|AS|JP>              # LoRa frequency region
CONFIG_DATA_MODE <SIMPLE|ADMIN>             # Display verbosity
CONFIG_RADIO_PROFILE <PROFILE>              # Radio optimization
CONFIG_MAX_HOPS <1-10>                      # Maximum mesh hops
CONFIG_SAVE                                 # Save configuration to EEPROM
CONFIG_RESET                                # Factory reset
START                                       # Begin operation
STATUS                                      # Show current configuration
INFO                                        # Device information
HELP                                        # Command reference
```

### Network Management Commands

```bash
NETWORK_CREATE <name> [password]            # Create encrypted network
NETWORK_JOIN <name> <password>              # Join existing network
NETWORK_LIST                                # Show all networks
NETWORK_INFO [name]                         # Network details
NETWORK_STATUS                              # Current network state
NETWORK_DELETE <name>                       # Remove network
```

### Operational Commands

```bash
MODE SIMPLE                                 # Switch to minimal display
MODE ADMIN                                  # Switch to detailed display
DISCOVER                                    # Find network devices (RECEIVER only)
REMOTE_CONFIG <deviceID>                    # Configure remote device
```

---

## Technical Specifications

### Meshtastic Algorithm Implementation

**Core Components:**
- **Duplicate Detection**: `wasSeenRecently(sourceID, packetID)` with 100-packet memory
- **SNR-based Delays**: `getTxDelayMsecWeighted(snr, role)` for collision avoidance
- **Role Priority**: REPEATER = base delay, TRACKER = base delay + 160ms
- **Hop Management**: Maximum 3 hops with automatic TTL decrement
- **Flood Routing**: Complete Meshtastic managed flood routing algorithm

**Network Statistics Tracking:**
```cpp
struct LoRaStats {
    uint32_t packetsSent;           // Packets transmitted
    uint32_t packetsReceived;       // Valid packets received
    uint32_t duplicatesIgnored;     // Duplicate packets filtered
    uint32_t rebroadcasts;          // Packets retransmitted
    uint32_t hopLimitReached;       // Packets dropped (max hops)
    uint32_t networkFiltered;       // Different network packets filtered
    float lastRSSI, lastSNR;        // Signal quality metrics
};
```

### Memory and Storage

**EEPROM Utilization:**
- **Configuration storage**: Device settings, network definitions
- **Network persistence**: Encrypted network credentials
- **Statistics**: Long-term performance metrics
- **Available space monitoring**: Automatic usage tracking

**Flash Memory Usage:**
- **Firmware code**: ~800KB
- **RadioLib library**: ~150KB
- **Available user space**: Sufficient for custom applications

---

## Documentation and Support

### Additional Resources

- [Meshtastic Protocol Documentation](https://meshtastic.org/) - Reference implementation
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 LoRa Transceiver](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)
- [LoRa Regional Parameters](https://lora-alliance.org/resource_hub/rp2-1-0-3-lorawan-regional-parameters/)

### Algorithm References

- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Development Team

**Product Manager/Engineer**: Gilberto Castro Sustaita  
**Software Engineer**: Bryan Caleb Martinez Cavazos
