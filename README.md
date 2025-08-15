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
- **Automated cross-platform deployment** via flash tool
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

## Installation and Deployment

### Automated Deployment (Recommended)

**Prerequisites:**
- Python 3.x installed
- ESP32-S3 device connected via USB-C
- Git repository cloned: `git clone https://github.com/Clarexz/meshtastic_custom.git`

**Single-command deployment:**

**Windows:**
```bash
python flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA
```

**Mac/Linux:**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA
```

**The tool automatically:**
1. Detects ESP32-S3 port across all platforms
2. Installs missing dependencies (pyserial, PlatformIO)
3. Compiles and flashes firmware
4. Configures all device parameters
5. Creates and activates encrypted network
6. Starts operational mode

### Deployment Parameters

| Parameter | Description | Valid Values | Example |
|-----------|-------------|--------------|---------|
| `-role` | Device function | TRACKER, REPEATER, RECEIVER | `TRACKER` |
| `-id` | Unique device ID | 1-999 | `1` |
| `-gps` | GPS transmission interval | 5-3600 seconds | `15` |
| `-region` | LoRa frequency region | US, EU, CH, AS, JP | `US` |
| `-mode` | Display verbosity | SIMPLE, ADMIN | `SIMPLE` |
| `-radio` | Radio optimization profile | See Radio Profiles section | `DESERT_LONG_FAST` |
| `-channel` | Private network name | 3-20 alphanumeric chars | `TEAM_ALPHA` |

### Network Security Examples

**Public network (no encryption):**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 30 -region US -mode SIMPLE -radio URBAN_DENSE -channel PUBLIC
```

**Private encrypted network:**
```bash
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode ADMIN -radio MESH_MAX_NODES -channel SECURE_OPS
```

**Multiple isolated networks:**
```bash
# Network A devices in north_team channel
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode ADMIN -radio MESH_MAX_NODES -channel NORTH_TEAM

# Network B devices (completely isolated because they are in a completely different channel)
python3 flash_tool.py -role TRACKER -id 1 -gps 15 -region US -mode ADMIN -radio MESH_MAX_NODES -channel SOUTH_TEAM
```

---

## Hardware Configuration

### Unified GPIO Template

The system uses a centralized GPIO management approach with all pin assignments unified in `user_logic.h`:

**System Peripherals (15 pins occupied):**

**GPS L76K Module:**
```cpp
#define GPS_RX_PIN 4
#define GPS_TX_PIN 5
```

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

**LED Matrix MAX7219 Display:**
```cpp
#define LED_MATRIX_DATA_PIN 2
#define LED_MATRIX_CS_PIN 3
#define LED_MATRIX_CLK_PIN 6
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

The system includes five pre-configured radio profiles optimized for different deployment scenarios:

| Profile | SF | Bandwidth | Power | Range | Airtime | Use Case |
|---------|----|-----------| ------|-------|---------|----------|
| **DESERT_LONG_FAST** | 11 | 250kHz | 20dBm | ~8km | ~2.2s | Open terrain, maximum range |
| **MOUNTAIN_STABLE** | 10 | 125kHz | 20dBm | ~4km | ~0.9s | Obstacles, harsh conditions |
| **URBAN_DENSE** | 7 | 500kHz | 14dBm | ~800m | ~80ms | High speed, development, testing |
| **MESH_MAX_NODES** | 9 | 250kHz | 17dBm | ~2.5km | ~320ms | Large networks (20-30 nodes) |
| **CUSTOM_ADVANCED** | Variable | Variable | Variable | Variable | Variable | Manual configuration |

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

---

## LoRa Regional Support

| Region | Code | Frequency | Coverage Areas |
|--------|------|-----------|----------------|
| **US** | `US` | 915 MHz | United States, Mexico, Canada |
| **EU** | `EU` | 868 MHz | Europe, Africa, Middle East |
| **CH** | `CH` | 470 MHz | China |
| **AS** | `AS` | 433 MHz | Asia (general) |
| **JP** | `JP` | 920 MHz | Japan |

---

## Data Visualization

### SIMPLE Mode (Clean Output)

**Purpose**: Minimal display showing only essential data to save energy resources

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

### Deployment Issues

**Flash tool cannot detect ESP32-S3:**
```bash
# Solutions:
1. Verify USB-C connection integrity
2. Temporarily disconnect LoRa module (potential interference)
3. Use manual BOOT+RESET sequence
4. Install/update USB drivers
5. Try different USB port or cable
```

**Python dependencies missing:**
```bash
# Automatic resolution:
# Flash tool automatically installs pyserial and detects PlatformIO
# If manual intervention needed:
pip install pyserial
# Install PlatformIO Core manually if auto-detection fails
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