# Custom LoRa Mesh Asset Tracking System

**Complete IoT mesh system with cloud connectivity and automated tools**

![Project Status](https://img.shields.io/badge/Status-Operational-brightgreen)
![Version](https://img.shields.io/badge/Version-0.4.0-blue)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Tools](https://img.shields.io/badge/Tools-Cross--Platform-green)

---

## Project Description

Off-grid GPS tracking system based on complete Meshtastic algorithm, evolved into a comprehensive IoT solution with cloud connectivity, automated development tools, and unified hardware management. The system implements an autonomous LoRa mesh network with remote configuration capabilities, optimized radio profiles, and cloud gateway for centralized visualization.

### Main Features

- **LoRa mesh system** with complete Meshtastic algorithm
- **Automated tools** for cross-platform flashing and configuration
- **Unified GPIO template** for managing all peripherals
- **Radio profiles** optimized for different environments
- **Mesh security** with device authentication and encrypted channels
- **Complete remote configuration** between devices
- **Multi-region support** LoRa (US/EU/CH/AS/JP)

---

## System Architecture

### System Roles

#### TRACKER
- Transmits real GPS position periodically
- Radio profile configuration according to environment

#### REPEATER
- Range extender for the mesh network
- Intelligent retransmission with SNR-based delays

#### END NODE REPEATER (PLANNED)
- Gateway to cloud via LTE connectivity
- Local storage for redundancy
- HTTPS/MQTTS uplink for remote visualization
- Data buffering during disconnections

#### RECEIVER
- Centralized control of the entire network
- Mass remote configuration of devices
- Automatic discovery of network devices

---

## Installation and Configuration

### Automated Method

**Prerequisites:**
- Python 3.x installed on the system
- ESP32-S3 connected via USB-C
- PlatformIO (auto-installation available in flashing tool)
- git clone https://github.com/Clarexz/meshtastic_custom.git

**Complete configuration in one line:**

**Windows:**
```bash
python flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel CAMELLOS_NORTE
```

**Mac/Linux:**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel CAMELLOS_NORTE
```

**The tool automatically executes:**
1. ESP32-S3 port detection
2. Installation of missing dependencies
3. Firmware compilation
4. Device flashing
5. Configuration of all parameters
6. **Network channel creation** (if specified)
7. Start of operational mode

### Configuration Parameters

```bash
-role <TRACKER|REPEATER|RECEIVER|END_NODE_REPEATER>  # Device role
-id <1-999>                                          # Unique ID in network
-gps <5-3600>                                        # GPS interval in seconds
-region <US|EU|CH|AS|JP>                            # LoRa region
-mode <SIMPLE|ADMIN>                                 # Visualization mode
-radio <PROFILE>                                     # Optimized radio profile
-hops <1-10>                                         # Maximum mesh hops
-channel <CHANNEL_NAME>                              # Private network channel (optional)
```

### Network Security Examples

**Public network (default):**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST
```

**Private encrypted network:**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel CAMELLOS_NORTE
```

**Benefits of private channels:**
- **Complete isolation** from other mesh networks
- **Automatic encryption** of all communications
- **Network segregation** (e.g., "CAMELLOS_NORTE" vs "LIEBRES_SUR")
- **Persistent configuration** across device reboots

### Manual Configuration

**For advanced development:**
```bash
git clone https://github.com/Clarexz/meshtastic_custom.git
cd custom-meshtastic-custom
# Open in VS Code with PlatformIO
# Manual Build and Upload
```

---

## Unified GPIO Template

### Complete Pin Assignment

**System Peripherals (15 pins occupied):**

**GPS L76K:**
```cpp
#define GPS_RX_PIN 4
#define GPS_TX_PIN 5
```

**LoRa SX1262:**
```cpp
#define LORA_SCK_PIN 7
#define LORA_MISO_PIN 8
#define LORA_MOSI_PIN 9
#define LORA_CS_PIN 41
#define LORA_DIO1_PIN 39
#define LORA_RST_PIN 42
#define LORA_BUSY_PIN 40
```

**LED Matrix MAX7219:**
```cpp
#define LED_MATRIX_DATA_PIN 2
#define LED_MATRIX_CS_PIN 3
#define LED_MATRIX_CLK_PIN 6
```

**System:**
```cpp
#define LED_PIN 21              // Status LED
#define BATTERY_ADC_PIN 0       // Battery monitor
#define UART_DEBUG_PIN 43/44    // Serial debug
```

**Available Pins for User (10 pins):**
- **Pin 1** (ADC capable)
- **Pins 10-18** (Digital GPIO)
- **Total ADC available**: 2 (pins 1, 10)

### Template Usage

```cpp
#include "user_logic.h"

// Example of available pin usage
int sensorValue = analogRead(1);      // Pin 1 (ADC)
digitalWrite(10, HIGH);               // Pin 10 (Digital)
```

---

## System Commands

### Configuration Commands

```bash
CONFIG_ROLE <TRACKER|REPEATER|RECEIVER>     # Device role
CONFIG_DEVICE_ID <1-999>                    # Unique ID
CONFIG_GPS_INTERVAL <5-3600>                # GPS interval
CONFIG_REGION <US|EU|CH|AS|JP>              # LoRa region
CONFIG_DATA_MODE <SIMPLE|ADMIN>             # Visualization mode
CONFIG_RADIO_PROFILE <PROFILE>              # Optimized radio profile
CONFIG_MAX_HOPS <1-10>                      # Maximum hops
CONFIG_SAVE                                 # Save configuration
CONFIG_RESET                                # Reset configuration
START                                       # Start operation
STATUS                                      # View current configuration
INFO                                        # Device information
HELP                                        # Command list
```

### Network Security Commands

```bash
NETWORK_CREATE <CHANNEL_NAME>               # Create private channel
NETWORK_JOIN <CHANNEL_NAME>                 # Join existing channel
NETWORK_LIST                                # List available channels
NETWORK_DELETE <CHANNEL_NAME>               # Delete channel
NETWORK_STATUS                              # Show active channel
```

### Commands During Operation

```bash
MODE SIMPLE                              # Change to simple view
MODE ADMIN                               # Change to complete view
DISCOVER                                 # Search devices (RECEIVER)
REMOTE_CONFIG <deviceID>                 # Configure remote device
```

### Remote Configuration Commands

**Inside REMOTE_CONFIG mode:**
```bash
REMOTE_GPS_INTERVAL <5-3600>             # Change GPS interval
REMOTE_DATA_MODE <SIMPLE|ADMIN>          # Change data mode
REMOTE_STATUS                            # Get status
REMOTE_REBOOT                            # Restart device
EXIT                                     # Exit remote configuration
```

---

## Radio Profiles

### Implemented Profiles

Radio profiles are **completely implemented** with LoRa configurations optimized for different scenarios.

| Profile | SF | BW | Power | Range | Airtime | Recommended Use |
|---------|----|----|-------|-------|---------|-----------------|
| **DESERT_LONG_FAST** | 11 | 250kHz | 20dBm | ~8km | ~2.2s | Open terrain, maximum range |
| **MOUNTAIN_STABLE** | 10 | 125kHz | 20dBm | ~4km | ~0.9s | Adverse conditions, obstacles |
| **URBAN_DENSE** | 7 | 500kHz | 14dBm | ~800m | ~80ms | High speed, testing, development |
| **MESH_MAX_NODES** | 9 | 250kHz | 17dBm | ~2.5km | ~320ms | Large networks (20-30 nodes) |
| **CUSTOM_ADVANCED** | Variable | Variable | Variable | Variable | Variable | Manual configuration |

### Profile Commands

**Basic configuration:**
```bash
CONFIG_RADIO_PROFILE DESERT_LONG_FAST    # Maximum range
CONFIG_RADIO_PROFILE MOUNTAIN_STABLE     # Adverse conditions  
CONFIG_RADIO_PROFILE URBAN_DENSE         # High speed
CONFIG_RADIO_PROFILE MESH_MAX_NODES      # Large networks
CONFIG_RADIO_PROFILE CUSTOM_ADVANCED     # Manual configuration
```

**Information commands:**
```bash
CONFIG_RADIO_PROFILE LIST                # List all profiles
CONFIG_RADIO_PROFILE INFO DESERT         # Detailed information
CONFIG_RADIO_PROFILE COMPARE             # Compare all profiles
```

**Custom configuration (requires CUSTOM_ADVANCED active):**
```bash
RADIO_PROFILE_CUSTOM SF 8               # Spreading Factor 7-12
RADIO_PROFILE_CUSTOM BW 125             # Bandwidth: 125, 250, 500 kHz  
RADIO_PROFILE_CUSTOM CR 5               # Coding Rate: 5, 6, 7, 8
RADIO_PROFILE_CUSTOM POWER 14           # TX Power: 2-20 dBm
RADIO_PROFILE_CUSTOM PREAMBLE 8         # Preamble length
RADIO_PROFILE_APPLY                     # Apply custom configuration
RADIO_PROFILE_STATUS                    # Show current configuration
```

---

## Network Security

### Private Mesh Networks

The system supports **completely isolated private networks** using encrypted channels:

**Key features:**
- **AES-256-CTR encryption** for all communications
- **Channel-based isolation** - devices on different channels cannot communicate
- **Persistent configuration** - channels survive device reboots
- **Automatic key management** - no manual PSK handling required

### Channel Management

**Create a private network:**
```bash
NETWORK_CREATE CAMELLOS_NORTE
[OK] Canal 'CAMELLOS_NORTE' creado exitosamente
[INFO] Canal guardado automáticamente en EEPROM
```

**Join an existing network:**
```bash
NETWORK_JOIN CAMELLOS_NORTE
[OK] Conectado al canal: CAMELLOS_NORTE
```

**List available channels:**
```bash
NETWORK_LIST
Canal 0: CAMELLOS_NORTE (ACTIVO)
Canal 1: LIEBRES_SUR
```

**Network isolation example:**
- Devices on channel "CAMELLOS_NORTE" can only communicate with other devices on the same channel
- Devices on channel "LIEBRES_SUR" form a completely separate mesh network
- Packets are automatically encrypted and filtered by channel

---

## LoRa Regions

| Region | Code | Frequency | Countries/Zones |
|--------|------|-----------|-----------------|
| **US** | `US` | 915 MHz | United States, Mexico, Canada |
| **EU** | `EU` | 868 MHz | Europe, Africa, Middle East |
| **CH** | `CH` | 470 MHz | China |
| **AS** | `AS` | 433 MHz | Asia general |
| **JP** | `JP` | 920 MHz | Japan |

---

## Visualization Modes

### SIMPLE Mode

**Purpose**: Clean view with essential information

**Output format:**
```
[001,25.302677,-98.277664,4100,1718661234]
Transmission completed
```

### ADMIN Mode

**Purpose**: Complete view with detailed statistics

**Output format:**
```
[TRACKER] === GPS + LoRa MESH TRANSMISSION ===
Device ID: 1
Role: TRACKER (CLIENT priority)
Region: US (United States/Mexico)
Frequency: 915.0 MHz
Radio Profile: DESERT_LONG_FAST
Network Channel: CAMELLOS_NORTE (ENCRYPTED)
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
Next transmission in 15 seconds
==========================================
```

---

## Packet Format

### Data Structure

**Basic GPS format:**
```
[deviceID, latitude, longitude, batteryvoltage, timestamp]
```

**Example:**
```
[001,25.302677,-98.277664,4100,1718661234]
```

### Packet Fields

| Field | Description | Format | Example |
|-------|-------------|--------|---------|
| `deviceID` | Device unique ID | 001-999 | `001` |
| `latitude` | Latitude in decimal degrees | 6 decimals | `25.302677` |
| `longitude` | Longitude in decimal degrees | 6 decimals | `-98.277664` |
| `batteryvoltage` | Battery voltage | millivolts (mV) | `4100` |
| `timestamp` | Unix timestamp | seconds | `1718661234` |

---

## Meshtastic Algorithm

### Implemented Components

**Duplicate Detection:**
```cpp
bool wasSeenRecently(sourceID, packetID);  // Memory of 100 packets
```

**SNR-based Delays:**
```cpp
uint32_t delay = getTxDelayMsecWeighted(snr, role);  // Distant nodes first
```

**Role Priority:**
```cpp
if (role == ROLE_REPEATER) delay = base_delay;       // REPEATER priority
else delay = base_delay + 160ms;                     // CLIENT priority
```

**Hop Management:**
```cpp
if (packet.hops >= packet.maxHops) drop_packet();    // Maximum 3 hops
```

### Network Statistics

```cpp
struct LoRaStats {
    uint32_t packetsSent;        // Packets sent
    uint32_t packetsReceived;    // Valid packets received
    uint32_t duplicatesIgnored;  // Filtered duplicates
    uint32_t rebroadcasts;       // Retransmissions performed
    uint32_t hopLimitReached;    // Packets dropped by hop limit
    float lastRSSI, lastSNR;     // Signal quality
};
```

---

## Troubleshooting

### Tool Issues

**Error: "ESP32-S3 not detected"**
```bash
# Solutions:
1. Verify USB-C connection
2. Temporarily disconnect LoRa module (interference)
3. Use manual BOOT+RESET sequence
4. Verify USB drivers installed
```

**Error: "PlatformIO not found"**
```bash
# Solutions:
1. Allow tool auto-installation
2. Verify Python 3.x installed
3. Run with administrative permissions
4. Install PlatformIO manually if necessary
```

### Configuration Issues

**Error: "Command not recognized"**
```bash
# Verifications:
1. Use automated tool for configuration
2. Correct command syntax
3. Device in configuration mode
```

**Error: "Channel creation failed"**
```bash
# Solutions:
1. Verify channel name is valid (no spaces, special characters)
2. Check if channel already exists with NETWORK_LIST
3. Ensure device has sufficient memory for new channels
```

### Network Issues

**Error: "No devices detected"**
```bash
# Verifications:
1. Same LoRa region on all devices
2. Same network channel on all devices
3. Devices within LoRa range
4. Verify compatible radio profiles
```

**Error: "Devices not communicating"**
```bash
# Solutions:
1. Verify all devices are on the same channel (NETWORK_STATUS)
2. Check channel configuration with NETWORK_LIST
3. Ensure network isolation is not blocking communication
4. Use DISCOVER to test connectivity
```

---

## Technical Documentation

### Additional Resources

- [Meshtastic Documentation](https://meshtastic.org/) - Original protocol reference
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [RadioLib Documentation](https://github.com/jgromes/RadioLib)
- [LoRa Regional Parameters](https://lora-alliance.org/resource_hub/rp2-1-0-3-lorawan-regional-parameters/)

### Meshtastic Algorithm

- [Managed Flood Routing Explained](https://meshtastic.org/blog/why-meshtastic-uses-managed-flood-routing/)
- [Mesh Broadcast Algorithm](https://meshtastic.org/docs/overview/mesh-algo/)

---

## Team

**Product Manager/Engineer**: Gilberto Castro Sustaita  
**Engineer**: Bryan Caleb Martinez Cavazos

---

**Last updated**: August 1, 2025  
**Firmware version**: 0.4.0