# Custodia Project Description

## Mission
Custodia is a self-contained LoRa mesh asset-tracking platform built on top of the Meshtastic managed flood routing algorithm. The firmware targets low-power Seeed XIAO boards (ESP32-S3 and nRF52840 variants) and orchestrates GPS telemetry, encrypted mesh networking, and multi-role device behavior (tracker, repeater, receiver). The repository also ships cross-platform tooling that turns board provisioning into a guided workflow covering firmware flashing, network bootstrapping, and ongoing remote administration.

## Core Capabilities
- Managed LoRa mesh networking with duplicate filtering, hop control, and role-weighted rebroadcast delays.
- Multiple device personas: GPS trackers broadcast position and battery status, repeaters extend coverage, and receivers aggregate, monitor, and issue remote commands.
- Encrypted private networks stored persistently with hashed identifiers, supporting quick reselection across up to ten networks per node.
- Rich serial command interface with quick configuration macros, admin/simple verbosity modes, and remote configuration channels.
- Automated flashing and provisioning scripts that validate inputs, install dependencies, detect boards/ports, and stream post-flash commands.
- Modular hardware abstraction layers for GPS, battery sensing, LED matrix display, and GPIO pin plans for each supported MCU family.

## Supported Hardware & Build Targets
- `seeed_xiao_esp32s3` (default PlatformIO environment) with Preferences-based NVS storage.
- `seeed_xiao_nrf52840` (Seeed Studio board package) using LittleFS for persistence.
- SX1262 LoRa radios via RadioLib 6.4.2 and MAX7219 LED matrix display modules.
- GPS module placeholder (L76K-friendly UART wiring) ready to be swapped with real sensor drivers.

## Repository Layout
| Path | Purpose |
| ---- | ------- |
| `src/` | Production firmware code split by subsystem (config, GPS, battery, LoRa stack, role logic, displays, serial/remote command handling). |
| `include/`, `lib/` | PlatformIO include/lib placeholders (currently documentation only). |
| `Flash Tool MacOS/`, `flash_tool.py`, `Custodia_Flash_Tool.py`, `flash_tool_macos.command` | Automation scripts for flashing and configuring devices on macOS, Windows, and Linux. |
| `meshtastic_radio_profiles.md` | Extended documentation describing each bundled radio profile. |
| `platformio.ini` | Build environments, shared flags, and dependency declarations. |
| `requirements.txt` | Python dependencies for the provisioning tools (pyserial, platformio). |
| `test/` | Placeholder for PlatformIO unit/integration tests (none implemented yet). |

## Firmware Architecture Overview
- **Entry point (`src/main.cpp`)**: Boots Serial at 115200 baud, initializes configuration storage, and dispatches to `RoleManager`. Runtime loop multiplexes serial input, deferred LoRa setup (after config changes), mesh updates, remote command processing, and LED heartbeat behavior tied to configuration state.
- **Configuration subsystem (`src/config/`)**:
  - `ConfigManager` owns `DeviceConfig`, system state machine (`STATE_BOOT`, `STATE_CONFIG_MODE`, `STATE_RUNNING`, `STATE_SLEEP`), and persistent storage. It works with ESP32 `Preferences` or NRF LittleFS behind the same API.
  - Supports up to ten stored networks (`SimpleNetwork`) with uppercase normalization, hashed credentials, and reserved-name/password-strength validation. Active network details are surfaced to the LoRa layer for packet filtering.
  - Command parsing is delegated to `config_commands.cpp` and `config_help.cpp`; quick macros like `Q_CONFIG` batch-set role, ID, GPS interval, region, mode, radio profile, and hop limit.
  - Tracks firmware version (`0.4.0`) and auto-applies the selected LoRa radio profile at boot.
- **Role coordination (`src/roles/`)**:
  - `RoleManager` orchestrates GPS and LoRa initialization per role and re-triggers bring-up if the radio reports errors.
  - `tracker_role`, `repeater_role`, and `receiver_role` implement role-specific loops, making heavy use of the GPS manager, LoRa manager, and display manager to present or relay data.
  - Receivers add a second state machine for remote configuration sessions, redirecting serial commands through `RemoteCommands`.
- **LoRa stack (`src/lora/` & `src/lora.h`)**:
  - `LoRaManager` wraps RadioLib `SX1262`, tracks radio status, maintains mesh statistics, and exposes send/receive helpers for GPS payloads, ACK/heartbeat, discovery, and remote config messages.
  - Implements Meshtastic managed flood routing verbatim: duplicate detection with a 100-packet sliding window, SNR-weighted contention windows, hop-limit enforcement (`MESHTASTIC_MAX_HOPS = 3`), and network-hash filtering to isolate private meshes.
  - Remote configuration flows cover discovery, command dispatch (`RemoteCommandType`), responses, and mesh-wide status queries. Message structs are in `lora_types.h` and are 32-byte payload conscious.
  - `lora_hardware` binds board-specific pin maps from `user_logic_*.h`; `lora_comm`, `lora_mesh`, `lora_packet`, and `lora_remote_config` split the stack by responsibility (transport, mesh logic, serialization, remote control).
- **GPS module (`src/gps/`)**: `GPSManager` presents a façade with interval control, coordinate formatting, and diagnostics. `gps_logic.cpp` currently simulates coordinates (incrementing counters) and documents the hook points for integrating a real UART-based GPS.
- **Battery monitoring (`src/battery/`)**: `BatteryManager` simulates discharge curves, calculates voltage/percentage, and delivers status helpers (low/critical) used in telemetry payloads.
- **Display layer (`src/display/`)**: `DisplayManager` fanouts to simple/admin renderers for tracker/repeater/receiver outputs and is designed around a MAX7219 display stack. Admin mode unlocks verbose mesh diagnostics.
- **Serial interfaces (`src/serial/`)**: `SerialHandler` enforces state-aware command routing and restricts operational commands when devices are running. `RemoteCommands` handles receiver-only macros like `DISCOVER`, remote configuration mode switching, and inbound remote command processing.
- **User logic shims (`src/user_logic*.h`)**: Define safe pin allocations for ESP32-S3 and nRF52840 targets, reserving critical buses and highlighting GPIO available for custom peripherals.

## Mesh Networking Details
- Duplicate suppression and sliding window retention mirror Meshtastic's `FloodingRouter`, trimming entries after five minutes (`PACKET_MEMORY_TIME`).
- Role-specific contention window weights give repeaters priority in rebroadcast scheduling (`ROLE_REPEATER` uses base delay, trackers/receivers add additive delay).
- Packets include `networkHash`; anything outside the active hash is skipped, contributing to `networkFilteredPackets` statistics. Remote configuration commands respect the same isolation.
- Default hop limit is three, adjustable via configuration. Packets hitting the cap increment `hopLimitReached` counters for diagnostics.
- Discovery messages advertise role, GPS cadence, data mode, region, battery, and uptime, aiding remote fleet management from a receiver node.

## Radio Profiles & Regions
- `radio_profiles.h` defines 13 named profiles tuned for terrain/spectrum trade-offs (desert, mountain, urban, mesh density, short/medium/long-range variants, plus a custom slot). Each profile specifies spreading factor, bandwidth, coding rate, TX power, and preamble length, and labels intended use cases and trade-offs.
- Regions (`US`, `EU`, `CH`, `AS`, `JP`) drive center frequency selection (915/868/470/433/920 MHz) and are enforced during config validation.
- `RadioProfileManager` applies profiles to the radio hardware and exposes parsing helpers for command handlers and automated tooling.

## Configuration & Operations
- **Device states**: `STATE_CONFIG_MODE` blinks the LED slowly and routes Serial to configuration handlers; `STATE_RUNNING` executes the role loop and processes mesh traffic; `STATE_SLEEP` is reserved for future power management.
- **Serial commands**: Core commands (`CONFIG_ROLE`, `CONFIG_DEVICE_ID`, `CONFIG_GPS_INTERVAL`, `CONFIG_REGION`, `CONFIG_DATA_MODE`, `CONFIG_RADIO_PROFILE`, `CONFIG_MAX_HOPS`, `MODE`, `CONFIG_SAVE`, `STATUS`, `INFO`, `HELP`) are documented in `README.md`. `Q_CONFIG` condenses common fields into a single comma-separated macro. `NETWORK_*` commands manage private networks with validation and persistence.
- **Runtime controls**: While running, devices accept a reduced command set (`MODE`, `CONFIG_RESET`, `CONFIG`, `STATUS`, `INFO`, `HELP`). Receiver nodes expose `DISCOVER`, `REMOTE_CONFIG`, and remote command dispatch through `RemoteCommands`.
- **Admin vs simple mode**: Admin mode enables verbose serial output (mesh stats, SNR-derived delays, filtered packet reasons), whereas simple mode favors concise telemetry for field deployments.

## Automation Tooling
- **`flash_tool.py`**: Comprehensive CLI that checks/installs `pyserial` and `platformio`, finds the PlatformIO executable, auto-detects ESP32 ports, validates command-line parameters, flashes the firmware, pushes configuration macros (`NETWORK_CREATE`, `Q_CONFIG`, `CONFIG_SAVE`, `START`), and opens a serial monitor in a new terminal window per OS. Includes extensive recovery messaging for manual intervention.
- **`Custodia_Flash_Tool.py`**: Pythonic reimplementation of the macOS shell workflow, featuring interactive prompts, colored console output, AppleScript relaunch support, and structured validation errors. Designed to align UX across packaged and CLI contexts.
- **`flash_tool_macos.command` / `Flash Tool MacOS/flash_tool_macos.sh`**: Shell-based launcher for macOS users preferring native app bundles; orchestrates the same provisioning flow using `pio` under the hood.
- All tools depend on `requirements.txt` and treat PlatformIO installation as part of the provisioning pipeline, making first-time setup smoother for non-developers.

## Build & Dependency Notes
- PlatformIO builds with Arduino framework, C++14 standard (`-std=gnu++14`), and high debug verbosity (`-DCORE_DEBUG_LEVEL=3`) for the ESP32 environment. USB CDC on boot is enabled to expose native serial over USB without external drivers.
- Shared dependency: `RadioLib@^6.4.2` for SX1262 control.
- ESP32 environment pulls in the `Preferences` library automatically; nRF environment relies on the Seeed board platform hosted on GitHub.
- GPS and battery modules are structured to replace simulation logic with real hardware drivers without touching consuming code (only `gps_logic.cpp` and `battery_manager.cpp`).

## Data Flow & Telemetry
- Tracker packets use the packed `GPSPayload` struct (latitude, longitude, timestamp, battery voltage, satellite count) wrapped in `MSG_GPS_DATA`. LoRa payloads are capped at 32 bytes to satisfy SX1262 limitations.
- `LoRaStats` aggregates packet counts, duplicates ignored, rebroadcasts, hop-limit drops, network filters, and last RSSI/SNR readings for both runtime diagnostics and future analytics.
- Battery percentage is derived from configurable min/max voltage thresholds (3.2V–4.2V). GPS timestamps default to `millis()/1000` until a real RTC/GPS driver populates them.

## Diagnostics & Error Handling
- LoRa initialization failures force a reattempt from `RoleManager` and echo errors to Serial. Admin mode logs detailed context (delays, filtered packets, network mismatches).
- Configuration storage bootstraps with verbose messaging and prompts if persistence is unavailable (e.g., LittleFS mount issues on nRF); configuration remains volatile in that scenario.
- Flashing scripts include verbose logs, capture stderr tail for PlatformIO errors, and suggest manual recovery sequences (reset/boot button instructions, exact commands to rerun).

## Extensibility & Integration Points
- Replace GPS simulation by wiring real sensor reads into `gps_logic.cpp` while keeping the `GPSManager` API untouched.
- Tie battery manager to ADC measurements by swapping the simulation in `battery_manager.cpp` with analog reads respecting `USER_ADC_*` pins.
- Extend display rendering in `simple_display.cpp` / `admin_display.cpp` to provide richer UI on the MAX7219 or alternative outputs.
- Add new remote command types by updating `RemoteCommandType`, serialization handlers in `lora_remote_config`, and receiver UI prompts.
- Introduce additional radio profiles through `radio_profiles.h` and ensure `RadioProfileManager` returns descriptive metadata.
- Implement PlatformIO Unity tests under `test/` once hardware abstractions are mockable; currently no automated tests are provided.

## Operational Checklist
- Ensure Python 3, PlatformIO, and pyserial are installed (automation scripts will attempt install/update but may require PATH adjustments).
- Choose the correct PlatformIO environment for the connected board: ESP32-S3 is default; override with `-e seeed_xiao_nrf52840` for nRF builds.
- After flashing, verify active network and role via `STATUS`/`INFO`. Receivers should issue `DISCOVER` to confirm mesh connectivity and remote configuration readiness.
- For multi-network deployments, use `NETWORK_CREATE`, `NETWORK_JOIN`, and `NETWORK_STATUS` to manage hashed credentials consistently across nodes.

## Known Gaps & Future Work
- GPS and battery managers currently use simulated data; hardware integration is pending.
- No automated tests or CI scripts exist beyond placeholders. Manual validation through serial monitoring and mesh field tests is required.
- Power management (`STATE_SLEEP`) and deep sleep handling are stubbed for future implementation.
- Display rendering assumes a MAX7219 stack; alternative display hardware will require driver additions.
- Remote configuration is centered on a receiver acting as network coordinator; other roles do not yet initiate remote commands.

