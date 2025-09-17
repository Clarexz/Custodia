#!/usr/bin/env python3
"""Custodia Flash Tool (Python)

Python port that aligns with the macOS shell flash tool UX. This block focuses on
collecting configuration parameters (interactive and command modes) together with
validation logic. Flashing and device configuration pipelines will follow next.
"""
from __future__ import annotations

import argparse
import os
import platform
import re
import shlex
import shutil
import site
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Sequence

# Terminal color codes to mirror the macOS shell script output style
RED = "\033[0;31m"
GREEN = "\033[0;32m"
YELLOW = "\033[1;33m"
BLUE = "\033[0;34m"
PURPLE = "\033[0;35m"
RESET = "\033[0m"

# Valid options shared across the CLI
VALID_ROLES = ["TRACKER", "RECEIVER", "REPEATER"]
VALID_REGIONS = ["US", "EU", "CH", "AS", "JP"]
VALID_MODES = ["SIMPLE", "ADMIN"]
VALID_RADIOS = [
    "DESERT_LONG_FAST",
    "MOUNTAIN_STABLE",
    "URBAN_DENSE",
    "MESH_MAX_NODES",
    "CUSTOM_ADVANCED",
]
RESERVED_CHANNELS = {
    "CONFIG",
    "ADMIN",
    "DEBUG",
    "SYSTEM",
    "DEVICE",
    "LORA",
    "MESH",
    "TEST",
    "DEFAULT",
}
DEFAULT_BOARD_ENV = "seeed_xiao_esp32s3"


class FlashToolError(Exception):
    """Custom exception used for recoverable validation errors."""


@dataclass
class FlashConfig:
    """Holds the configuration values collected from the user."""

    role: Optional[str] = None
    device_id: Optional[int] = None
    interval: Optional[int] = None
    region: Optional[str] = None
    mode: Optional[str] = None
    radio: Optional[str] = None
    hops: int = 3
    channel: Optional[str] = None
    password: Optional[str] = None
    port: Optional[str] = None
    pio_cmd: Optional[str] = None
    board_env: str = DEFAULT_BOARD_ENV
    custom_params: List[str] = field(default_factory=list)
    network_hash: Optional[str] = None

    def as_readable_summary(self) -> str:
        """Return a human-friendly multi-line summary of the current configuration."""
        lines = [
            f"Role: {self.role}",
            f"Device ID: {self.device_id}",
            f"Transmission interval: {self.interval} seconds",
            f"Region: {self.region}",
            f"Mode: {self.mode}",
            f"Radio: {self.radio}",
            f"Hops: {self.hops}",
            f"Channel: {self.channel}",
            f"Password: {self.password}",
        ]
        if self.custom_params:
            lines.append(f"Custom radio parameters: {', '.join(self.custom_params)}")
        if self.port:
            lines.append(f"Serial port: {self.port}")
        if self.network_hash:
            lines.append(f"Network hash: {self.network_hash}")
        return "\n".join(lines)


# ---------------------------------------------------------------------------
# Console helpers
# ---------------------------------------------------------------------------

def color_text(color: str, message: str) -> str:
    """Wrap a message with ANSI color codes when stdout is a TTY."""
    if sys.stdout.isatty():
        return f"{color}{message}{RESET}"
    return message


def print_color(color: str, message: str) -> None:
    """Print a message using the requested color."""
    print(color_text(color, message))


def print_header() -> None:
    """Render the standard Custodia Flash Tool heading."""
    os.system("clear" if platform.system() != "Windows" else "cls")
    print_color(BLUE, "==================================")
    print_color(BLUE, "    Custodia Flash Tool v2.0     ")
    print_color(BLUE, "        Python Interactive CLI   ")
    print_color(BLUE, "==================================")
    print()


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def get_paths() -> tuple[Path, Path]:
    """Return (script_dir, project_dir) akin to the shell implementation."""
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir
    return script_dir, project_dir


def format_command(parts: Sequence[str]) -> str:
    """Return an OS-appropriate string representation of a command."""
    if not parts:
        return ""

    if os.name == "nt":
        def quote_windows(arg: str) -> str:
            value = str(arg)
            if not value:
                return '""'
            needs_quotes = any(char in value for char in " 	&|^<>")
            value = value.replace('"', r'\"')
            if needs_quotes or value.startswith('"') or value.endswith('"'):
                return f'"{value}"'
            return value

        return " ".join(quote_windows(item) for item in parts)

    return " ".join(shlex.quote(str(item)) for item in parts)


# ---------------------------------------------------------------------------
# Dependency management and environment detection
# ---------------------------------------------------------------------------


def ensure_pyserial() -> "module":
    """Ensure pyserial is available, installing it if necessary."""
    try:
        import serial  # type: ignore
        import serial.tools.list_ports  # type: ignore  # noqa: F401
        return serial
    except ImportError:
        print_color(YELLOW, "PySerial not detected. Installing pyserial...")
        try:
            subprocess.run(
                [sys.executable, "-m", "pip", "install", "pyserial"],
                check=True,
                capture_output=True,
                text=True,
            )
        except subprocess.CalledProcessError as exc:
            stderr = exc.stderr.strip() if exc.stderr else ""
            raise FlashToolError(
                f"Failed to install pyserial automatically. Details: {stderr}"
            ) from exc
    try:  # Second chance after attempting installation
        import serial  # type: ignore
        import serial.tools.list_ports  # type: ignore  # noqa: F401
        print_color(GREEN, "pyserial installed successfully.")
        return serial
    except ImportError as exc:  # pragma: no cover - defensive
        raise FlashToolError("Could not import pyserial after installation.") from exc


def find_platformio() -> Optional[str]:
    """Locate a working PlatformIO executable similar to the macOS script."""
    names = ['pio', 'platformio']
    for name in names:
        candidate = shutil.which(name)
        if candidate and _pio_works(candidate):
            return candidate

    home = Path.home()
    candidates: List[Path] = []
    if os.name == 'nt':
        user_profile = Path(os.environ.get('USERPROFILE', str(home)))
        candidates.extend([
            user_profile / '.platformio' / 'penv' / 'Scripts' / 'pio.exe',
            user_profile / '.platformio' / 'penv' / 'Scripts' / 'platformio.exe',
        ])
        try:
            user_base = Path(site.getuserbase())
        except Exception:
            user_base = user_profile / 'AppData' / 'Roaming' / 'Python'
        candidates.extend([
            user_base / 'Scripts' / 'pio.exe',
            user_base / 'Scripts' / 'platformio.exe',
        ])
        python_dir = Path(sys.executable).resolve().parent
        candidates.extend([
            python_dir / 'pio.exe',
            python_dir / 'platformio.exe',
            python_dir / 'Scripts' / 'pio.exe',
            python_dir / 'Scripts' / 'platformio.exe',
            python_dir.parent / 'Scripts' / 'pio.exe',
            python_dir.parent / 'Scripts' / 'platformio.exe',
        ])
    else:
        candidates.extend([
            home / '.local' / 'bin' / 'pio',
            home / '.local' / 'bin' / 'platformio',
            home / '.platformio' / 'penv' / 'bin' / 'pio',
            home / '.platformio' / 'penv' / 'bin' / 'platformio',
            Path('/usr/local/bin/pio'),
            Path('/usr/local/bin/platformio'),
            Path('/opt/homebrew/bin/pio'),
            Path('/opt/homebrew/bin/platformio'),
        ])

    seen: set[str] = set()
    for candidate in candidates:
        candidate_str = str(candidate)
        if candidate_str in seen:
            continue
        seen.add(candidate_str)
        if candidate.exists() and _pio_works(candidate_str):
            return candidate_str
    return None




def _pio_works(executable: str) -> bool:
    try:
        result = subprocess.run(
            [executable, "--version"],
            capture_output=True,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.SubprocessError):
        return False
    return result.returncode == 0


def ensure_platformio() -> str:
    """Ensure PlatformIO is installed, matching the behaviour of the shell tool."""
    pio_cmd = find_platformio()
    if pio_cmd:
        print_color(GREEN, f"PlatformIO detected: {pio_cmd}")
        return pio_cmd

    print_color(YELLOW, "PlatformIO not found. Attempting installation via pip...")
    try:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "platformio"],
            check=True,
            capture_output=True,
            text=True,
        )
        time.sleep(2)
    except subprocess.CalledProcessError as exc:
        stderr = exc.stderr.strip() if exc.stderr else ""
        raise FlashToolError(
            "PlatformIO installation failed. "
            "Manual installation required (pip install platformio or brew install platformio)."
            f"\nDetails: {stderr}"
        ) from exc

    pio_cmd = find_platformio()
    if not pio_cmd:
        if os.name == "nt":
            user_profile = Path(os.environ.get("USERPROFILE", str(Path.home())))
            hint_candidates = [
                user_profile / ".platformio" / "penv" / "Scripts",
            ]
            try:
                hint_candidates.append(Path(site.getuserbase()) / "Scripts")
            except Exception:
                appdata = Path(os.environ.get("APPDATA", user_profile / "AppData" / "Roaming"))
                hint_candidates.append(appdata / "Python" / "Scripts")
            hint = " or ".join(dict.fromkeys(str(candidate) for candidate in hint_candidates))
        else:
            hint = "~/.local/bin or ~/.platformio/penv/bin"
        raise FlashToolError(
            "PlatformIO installation completed but executable not found. "
            f"Add {hint} to PATH and retry."
        )

    print_color(GREEN, f"PlatformIO ready: {pio_cmd}")
    return pio_cmd


def list_esp32_ports(serial_module: "module") -> List[tuple[str, str]]:
    """Return a list of (device, description) pairs that look like ESP32 boards."""
    try:
        ports = list(serial_module.tools.list_ports.comports())
    except Exception as exc:  # pragma: no cover - serial backend issues
        raise FlashToolError(f"Could not enumerate serial ports: {exc}")

    esp32_candidates: List[tuple[str, str]] = []
    skip_patterns = {"bluetooth", "debug-console", "hub", "keyboard", "mouse"}
    indicators = {
        "cp210",
        "ch340",
        "ch341",
        "ft232",
        "pl2303",
        "esp32",
        "serial",
        "uart",
        "usb-serial",
        "ttyusb",
        "ttyacm",
        "usbserial",
        "usbmodem",
    }

    for port in ports:
        device = port.device
        description = port.description or ""
        manufacturer = (port.manufacturer or "").lower()
        desc_lower = description.lower()
        device_lower = device.lower()

        if any(pattern in desc_lower for pattern in skip_patterns):
            continue

        is_candidate = False
        if any(tok in device_lower for tok in {"ttyusb", "ttyacm", "usbserial", "usbmodem"}):
            is_candidate = True
        if any(tok in desc_lower for tok in indicators):
            is_candidate = True
        if any(tok in manufacturer for tok in {"silicon labs", "ftdi", "prolific", "ch340"}):
            is_candidate = True

        if is_candidate:
            esp32_candidates.append((device, description))

    return esp32_candidates


def select_port_interactively(serial_module: "module") -> str:
    """Prompt the user to select an ESP32 port, mirroring the macOS experience."""
    candidates = list_esp32_ports(serial_module)
    if not candidates:
        print_color(YELLOW, "No ESP32 devices detected automatically.")
        all_ports = list(serial_module.tools.list_ports.comports())
        if not all_ports:
            raise FlashToolError("No serial ports available. Connect your device and retry.")

        print_color(BLUE, "Available ports:")
        for index, port in enumerate(all_ports, 1):
            print_color(PURPLE, f"  {index}. {port.device} - {port.description}")

        selection = input("Select port number or 'q' to cancel: ").strip()
        if selection.lower() == "q":
            raise FlashToolError("Operation cancelled by user.")
        if selection.isdigit():
            idx = int(selection) - 1
            if 0 <= idx < len(all_ports):
                return all_ports[idx].device
        raise FlashToolError("Invalid selection. Restart the tool and try again.")

    if len(candidates) == 1:
        device, description = candidates[0]
        print_color(GREEN, f"ESP32 detected: {device} - {description}")
        return device

    print_color(YELLOW, "Multiple ESP32 devices found:")
    for index, (device, description) in enumerate(candidates, 1):
        print_color(PURPLE, f"  {index}. {device} - {description}")

    while True:
        selection = input(f"Select port [1-{len(candidates)}]: ").strip()
        if selection.isdigit():
            idx = int(selection) - 1
            if 0 <= idx < len(candidates):
                chosen = candidates[idx][0]
                print_color(GREEN, f"Selected port: {chosen}")
                return chosen
        print_color(RED, "Invalid selection. Try again.")


def ensure_port(config: FlashConfig, serial_module: "module") -> str:
    """Ensure the configuration has a serial port, requesting user input if needed."""
    if config.port:
        print_color(GREEN, f"Using provided port: {config.port}")
        return config.port

    port = select_port_interactively(serial_module)
    config.port = port
    return port


# ---------------------------------------------------------------------------
# PlatformIO project helpers
# ---------------------------------------------------------------------------


def verify_project_structure(project_dir: Path) -> None:
    print_color(BLUE, "Verifying project structure...")
    platformio_ini = project_dir / "platformio.ini"
    if not platformio_ini.exists():
        raise FlashToolError(
            "platformio.ini not found in project directory. "
            f"Expected at: {platformio_ini}"
        )
    src_dir = project_dir / "src"
    if not src_dir.exists():
        print_color(YELLOW, "Warning: src directory not found. Continuing anyway.")
    print_color(GREEN, "Project structure verified successfully.")


def detect_board_type(config: FlashConfig) -> str:
    # Placeholder for future automatic detection; keeps parity with macOS script
    print_color(BLUE, "Using board environment: XIAO ESP32S3")
    config.board_env = DEFAULT_BOARD_ENV
    return config.board_env


def run_platformio_command(
    config: FlashConfig,
    project_dir: Path,
    args: Sequence[str],
    description: str,
    allow_failure: bool = False,
) -> subprocess.CompletedProcess:
    if not config.pio_cmd:
        raise FlashToolError("PlatformIO command not configured.")

    command = [config.pio_cmd, *args]
    print_color(BLUE, f"Executing: {' '.join(command)}")
    try:
        result = subprocess.run(
            command,
            cwd=str(project_dir),
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        raise FlashToolError(f"Failed to run PlatformIO ({description}): {exc}") from exc

    if result.returncode != 0 and not allow_failure:
        stderr_tail = (result.stderr or "").strip()[-400:]
        raise FlashToolError(
            f"PlatformIO command failed during {description}.\n"
            f"Return code: {result.returncode}\n"
            f"Output:\n{stderr_tail}"
        )

    return result


def clean_device_flash(config: FlashConfig, project_dir: Path) -> None:
    print_color(YELLOW, "\n[1/3] Cleaning device flash...")
    args = [
        "run",
        "-e",
        config.board_env,
        "--target",
        "erase",
        "--upload-port",
        config.port,
    ]
    try:
        run_platformio_command(config, project_dir, args, "flash erase", allow_failure=True)
        print_color(GREEN, "Flash erase command executed (ignored errors if unsupported).")
    except FlashToolError as exc:
        print_color(YELLOW, f"Flash erase step skipped: {exc}")
        print_color(YELLOW, "Continuing without full erase. Previous settings may persist.")


def flash_firmware(config: FlashConfig, project_dir: Path) -> None:
    print_color(YELLOW, "\n[2/3] Flashing firmware...")
    args = [
        "run",
        "-e",
        config.board_env,
        "--target",
        "upload",
        "--upload-port",
        config.port,
    ]
    try:
        run_platformio_command(config, project_dir, args, "firmware upload")
    except FlashToolError as exc:
        manual = (
            "Manual recovery steps:\n"
            "  1. Close any open serial monitors.\n"
            "  2. Hold BOOT button.\n"
            "  3. Press and release RESET.\n"
            "  4. Release BOOT.\n"
            f"  5. Run: {config.pio_cmd} run -e {config.board_env} --target upload --upload-port {config.port}\n"
        )
        raise FlashToolError(f"Firmware upload failed.\n{manual}\nDetails: {exc}")
    print_color(GREEN, "Firmware flashed successfully.")


def wait_for_device_reboot(delay_seconds: int = 8) -> None:
    print_color(BLUE, f"Waiting {delay_seconds} seconds for device reboot...")
    time.sleep(delay_seconds)


def build_serial_commands(config: FlashConfig) -> tuple[str, str, str]:
    channel = (config.channel or "").upper()
    password = (config.password or "").upper()
    role = (config.role or "").upper()
    region = (config.region or "").upper()
    mode = (config.mode or "").upper()
    radio = (config.radio or "").upper()
    hops = str(config.hops)

    network_cmd = f"NETWORK_CREATE {channel} {password}".strip()

    if radio == "CUSTOM_ADVANCED":
        bw, sf, cr, power, preamble = config.custom_params
        config_cmd = (
            f"Q_CONFIG {role},{config.device_id},{config.interval},{region},"
            f"{mode},{radio},{hops},{bw},{sf},{cr},{power},{preamble}"
        )
    else:
        config_cmd = (
            f"Q_CONFIG {role},{config.device_id},{config.interval},"
            f"{region},{mode},{radio},{hops}"
        )

    alt_cmd = (
        f"CONFIG {role},{config.device_id},{config.interval},"
        f"{region},{mode},{radio},{hops}"
    )
    return network_cmd, config_cmd, alt_cmd


def _read_serial_buffer(ser, delay: float = 0.1, max_wait: float = 2.0) -> str:
    """Read available data from serial buffer within max_wait seconds."""
    deadline = time.time() + max_wait
    chunks: List[str] = []
    while time.time() < deadline or ser.in_waiting:
        if ser.in_waiting:
            chunk = ser.read(ser.in_waiting)
            if chunk:
                chunks.append(chunk.decode("utf-8", errors="ignore"))
        time.sleep(delay)
    return "".join(chunks)


def configure_device(config: FlashConfig, serial_module: "module") -> None:
    print_color(YELLOW, "\n[3/3] Configuring device...")
    ser = None

    try:
        try:
            ser = serial_module.Serial(config.port, 115200, timeout=15)
        except Exception as exc:
            print_color(YELLOW, "Failed to open provided port. Re-detecting...")
            config.port = select_port_interactively(serial_module)
            ser = serial_module.Serial(config.port, 115200, timeout=15)

        time.sleep(5)
        ser.reset_input_buffer()

        print_color(BLUE, "Testing device communication...")
        ser.write(b"\r\n")
        time.sleep(1)
        ser.write(b"STATUS\r\n")
        time.sleep(2)
        _read_serial_buffer(ser)

        network_cmd, config_cmd, alt_cmd = build_serial_commands(config)
        print_color(BLUE, f"Creating network: {network_cmd}")
        ser.reset_input_buffer()
        ser.write((network_cmd + "\r\n").encode())
        time.sleep(4)
        network_response = _read_serial_buffer(ser, max_wait=2.5)
        print_color(BLUE, f"Network response: {network_response[:200]}")

        print_color(BLUE, f"Configuring device: {config_cmd}")
        ser.reset_input_buffer()
        ser.write((config_cmd + "\r\n").encode())
        time.sleep(4)
        config_response = _read_serial_buffer(ser, max_wait=2.5)
        print_color(BLUE, f"Config response: {config_response[:200]}")

        if any(token in config_response.lower() for token in {"comando desconocido", "unknown command"}) or len(config_response.strip()) < 10:
            print_color(YELLOW, "Primary config command not recognized, trying fallback CONFIG...")
            ser.reset_input_buffer()
            ser.write((alt_cmd + "\r\n").encode())
            time.sleep(4)
            alt_response = _read_serial_buffer(ser, max_wait=2.5)
            config_response += alt_response
            print_color(BLUE, f"Fallback response: {alt_response[:200]}")

        print_color(BLUE, "Saving configuration to EEPROM...")
        ser.reset_input_buffer()
        ser.write(b"CONFIG_SAVE\r\n")
        time.sleep(3)
        save_response = _read_serial_buffer(ser, max_wait=2.0)
        print_color(BLUE, f"Save response: {save_response[:200]}")

        print_color(BLUE, "Starting device operation...")
        ser.reset_input_buffer()
        ser.write(b"START\r\n")
        time.sleep(2)
        ser.write(b"NETWORK_STATUS\r\n")
        time.sleep(2)
        final_response = _read_serial_buffer(ser, max_wait=2.5)
        combined = (network_response + config_response + save_response + final_response).lower()
        success_tokens = {
            "network creada",
            "configuracion guardada",
            "configuración guardada",
            "network activa",
            "listo y operando",
        }
        if any(token in combined for token in success_tokens):
            print_color(GREEN, "Configuration completed successfully.")
        else:
            print_color(YELLOW, "Configuration may require manual verification.")
            print_manual_verification(config)

    except Exception as exc:
        print_manual_configuration(config)
        raise FlashToolError(f"Configuration error: {exc}") from exc
    finally:
        if ser is not None:
            try:
                ser.close()
            except Exception:
                pass


def print_manual_configuration(config: FlashConfig) -> None:
    print_color(YELLOW, "\n=== MANUAL RECOVERY ===")
    print_color(YELLOW, f"1. Connect manually: {config.pio_cmd} device monitor --port {config.port} --baud 115200")
    print_color(YELLOW, "2. Run the following commands:")
    network_cmd, config_cmd, alt_cmd = build_serial_commands(config)
    print_color(YELLOW, f"   STATUS")
    print_color(YELLOW, f"   {network_cmd}")
    print_color(YELLOW, f"   {config_cmd}")
    print_color(YELLOW, f"   {alt_cmd}  # if Q_CONFIG fails")
    print_color(YELLOW, "   CONFIG_SAVE")
    print_color(YELLOW, "   START")
    print_color(YELLOW, "   NETWORK_STATUS")


def print_manual_verification(config: FlashConfig) -> None:
    print_color(YELLOW, "Manual verification suggested:")
    print_color(YELLOW, f"1. Connect: {config.pio_cmd} device monitor --port {config.port} --baud 115200")
    print_color(YELLOW, "2. Check status: STATUS")
    print_color(YELLOW, "3. Check network: NETWORK_STATUS")


def launch_monitor(config: FlashConfig, project_dir: Path) -> None:
    if not config.pio_cmd:
        raise FlashToolError("PlatformIO executable not resolved; monitor launch aborted.")

    monitor_args: List[str] = [
        config.pio_cmd,
        "device",
        "monitor",
        "--baud",
        "115200",
    ]
    if config.port:
        monitor_args.extend(["--port", config.port])

    monitor_cmd = format_command(monitor_args)

    print_color(YELLOW, "\nLaunching serial monitor...")

    system = platform.system()
    if system == "Darwin":
        with tempfile.NamedTemporaryFile("w", delete=False, suffix=".sh") as script:
            script_path = Path(script.name)
            script.write(
                "#!/bin/bash\n"
                f"cd '{project_dir}'\n"
                "export PATH=\"$PATH:$HOME/.local/bin:$HOME/.platformio/penv/bin\"\n"
                "echo 'Custodia Device Monitor'\n"
                "echo '======================'\n"
                f"echo 'Port: {config.port}'\n"
                f"echo 'PlatformIO: {config.pio_cmd}'\n"
                "echo ''\n"
                f"{monitor_cmd}\n"
                "echo ''\n"
                "echo 'Monitor session ended. Press Enter to close...'\n"
                "read\n"
                "rm -f -- \"$0\"\n"
            )
        script_path.chmod(0o755)
        osa_cmd = [
            "osascript",
            "-e",
            f"tell application \"Terminal\" to do script \"{script_path}\"",
        ]
        try:
            subprocess.run(osa_cmd, check=True, capture_output=True)
            print_color(GREEN, "Monitor window opened in Terminal.")
        except subprocess.CalledProcessError as exc:
            print_color(YELLOW, f"Could not launch Terminal automatically: {exc}")
            print_color(YELLOW, f"Run manually: {monitor_cmd}")
            try:
                script_path.unlink(missing_ok=True)
            except OSError:
                pass
    elif system == "Windows":
        script_path: Optional[Path] = None
        with tempfile.NamedTemporaryFile("w", delete=False, suffix=".cmd") as script:
            script_path = Path(script.name)
            script.write("@echo off\r\n")
            script.write("setlocal enableextensions\r\n")
            script.write(f"cd /d \"{project_dir}\"\r\n")
            script.write("title Custodia Device Monitor\r\n")
            script.write("echo Custodia Device Monitor\r\n")
            script.write("echo ======================\r\n")
            script.write(f"echo Port: {config.port or '(auto)'}\r\n")
            script.write(f"echo PlatformIO: {config.pio_cmd}\r\n")
            script.write("echo.\r\n")
            script.write("set \"PATH=%PATH%;%USERPROFILE%\\.platformio\\penv\\Scripts;%APPDATA%\\Python\\Scripts\"\r\n")
            script.write(monitor_cmd + "\r\n")
            script.write("echo.\r\n")
            script.write("echo Monitor session ended. Press any key to close...\r\n")
            script.write("pause >nul\r\n")
            script.write("endlocal\r\n")
            script.write("del \"%~f0\"\r\n")
        try:
            if script_path is not None:
                os.startfile(str(script_path))  # type: ignore[attr-defined]
                print_color(GREEN, "Monitor window opened in Command Prompt.")
            else:
                raise OSError("Temporary script path unavailable.")
        except OSError as exc:
            print_color(YELLOW, f"Could not launch Command Prompt automatically: {exc}")
            print_color(YELLOW, f"Run manually: {monitor_cmd}")
            if script_path is not None:
                try:
                    script_path.unlink()
                except OSError:
                    pass
    else:
        print_color(YELLOW, "Automatic monitor launch is not implemented for this platform.")
        print_color(YELLOW, f"Run manually: {monitor_cmd}")




def show_final_summary(config: FlashConfig) -> None:
    print_color(BLUE, "\n===== CONFIGURATION SUMMARY =====")
    print_color(GREEN, f"Device ID: {config.device_id}")
    print_color(GREEN, f"Role: {config.role}")
    print_color(GREEN, f"Transmission Interval: {config.interval} seconds")
    print_color(GREEN, f"Region: {config.region}")
    print_color(GREEN, f"Mode: {config.mode}")
    print_color(GREEN, f"Radio Profile: {config.radio}")
    if config.radio == "CUSTOM_ADVANCED":
        bw, sf, cr, power, preamble = config.custom_params
        print_color(GREEN, f"  Bandwidth: {bw} kHz")
        print_color(GREEN, f"  Spreading Factor: SF{sf}")
        print_color(GREEN, f"  Coding Rate: 4/{cr}")
        print_color(GREEN, f"  TX Power: {power} dBm")
        print_color(GREEN, f"  Preamble: {preamble}")
    print_color(GREEN, f"Max Hops: {config.hops}")
    print_color(GREEN, f"Network Channel: {config.channel}")
    print_color(GREEN, f"Network Password: {config.password}")
    if config.network_hash:
        print_color(GREEN, f"Network Hash: {config.network_hash}")
    print_color(BLUE, "==================================")
    print_color(BLUE, "STATUS: READY FOR OPERATION")


# ---------------------------------------------------------------------------
# Validation utilities
# ---------------------------------------------------------------------------

def ensure_choice(
    prompt: str,
    options: List[str],
    display: Optional[List[tuple[str, str]]] = None,
) -> str:
    """Prompt user for a choice, optionally using a pre-rendered display menu."""

    options_map = {str(index): value for index, value in enumerate(options, 1)}
    options_lookup = {value.upper(): value for value in options}

    def print_menu() -> None:
        if display:
            for color, line in display:
                print_color(color, line)
        else:
            for index, value in options_map.items():
                print_color(PURPLE, f"  {index}. {value}")

    print_color(BLUE, prompt)
    print_menu()

    while True:
        choice = input("Select option: ").strip()
        if choice in options_map:
            return options_map[choice]
        normalized = choice.upper()
        if normalized in options_lookup:
            return options_lookup[normalized]
        print_color(RED, "Invalid selection. Try again.")
        print_menu()


def ensure_int(prompt: str, min_value: int, max_value: int) -> int:
    """Ensure the user provides an integer in a given range."""
    while True:
        raw = input(prompt).strip()
        if raw.isdigit():
            value = int(raw)
            if min_value <= value <= max_value:
                return value
        print_color(RED, f"Enter a number between {min_value} and {max_value}.")


def validate_channel(name: str) -> bool:
    if not 3 <= len(name) <= 20:
        raise FlashToolError("Channel name must be 3-20 characters long.")
    if name.upper() in RESERVED_CHANNELS:
        raise FlashToolError("Channel name is reserved. Choose a different one.")
    if not re.match(r"^[A-Z0-9_]+$", name):
        raise FlashToolError("Channel must contain only letters, numbers, or underscore.")
    return True


def validate_password(channel: str, password: str) -> bool:
    if not 8 <= len(password) <= 32:
        raise FlashToolError("Password must be 8-32 characters long.")
    if channel.upper() == password.upper():
        raise FlashToolError("Password cannot be the same as the channel name.")
    if not re.search(r"[A-Z]", password):
        raise FlashToolError("Password must contain at least one letter.")
    if not re.search(r"\d", password):
        raise FlashToolError("Password must contain at least one number.")
    return True


def generate_password() -> str:
    """Generate a random password following the project requirements."""
    import random
    import string

    letters = string.ascii_uppercase
    digits = string.digits
    all_chars = letters + digits
    password_chars = [random.choice(letters), random.choice(digits)]
    password_chars.extend(random.choice(all_chars) for _ in range(6))
    random.shuffle(password_chars)
    return "".join(password_chars)


# ---------------------------------------------------------------------------
# Interactive collectors
# ---------------------------------------------------------------------------

def collect_role() -> str:
    print_color(BLUE, "Device Role:")
    display = [
        (PURPLE, "  1. TRACKER  - GPS tracking device"),
        (PURPLE, "  2. RECEIVER - Base station/receiver"),
        (PURPLE, "  3. REPEATER - Signal repeater"),
    ]
    return ensure_choice("Select role [1-3]:", VALID_ROLES, display)


def collect_device_id() -> int:
    print_color(BLUE, "Device ID (1-255):")
    return ensure_int("Enter device ID: ", 1, 255)


def collect_interval() -> int:
    print_color(BLUE, "Transmission Interval (seconds, 5-3600):")
    return ensure_int("Enter interval: ", 5, 3600)


def collect_region() -> str:
    print_color(BLUE, "LoRa Region:")
    display = [
        (PURPLE, "  1. US - United States"),
        (PURPLE, "  2. EU - Europe"),
        (PURPLE, "  3. CH - China"),
        (PURPLE, "  4. AS - Asia"),
        (PURPLE, "  5. JP - Japan"),
    ]
    return ensure_choice("Select region [1-5]:", VALID_REGIONS, display)


def collect_mode() -> str:
    print_color(BLUE, "Operation Mode:")
    display = [
        (PURPLE, "  1. SIMPLE"),
        (PURPLE, "  2. ADMIN"),
    ]
    return ensure_choice("Select mode [1-2]:", VALID_MODES, display)


def collect_radio(config: FlashConfig) -> str:
    print_color(BLUE, "Radio Profile:")
    display = [
        (PURPLE, "  1. DESERT_LONG_FAST"),
        (PURPLE, "  2. MOUNTAIN_STABLE"),
        (PURPLE, "  3. URBAN_DENSE"),
        (PURPLE, "  4. MESH_MAX_NODES"),
        (PURPLE, "  5. CUSTOM_ADVANCED"),
    ]
    choice = ensure_choice("Select radio [1-5]:", VALID_RADIOS, display)
    if choice == "CUSTOM_ADVANCED":
        config.custom_params = collect_custom_params()
    return choice


def collect_custom_params() -> List[str]:
    print_color(YELLOW, "Custom Advanced Radio Parameters:")
    print_color(BLUE, "Bandwidth (kHz):")
    bandwidth = ensure_choice(
        "Select bandwidth [1-3]:",
        ["125", "250", "500"],
        [
            (PURPLE, "  1. 125 kHz (default)"),
            (PURPLE, "  2. 250 kHz"),
            (PURPLE, "  3. 500 kHz"),
        ],
    )

    print_color(BLUE, "Spreading Factor:")
    spreading_factor = ensure_choice(
        "Select spreading factor [1-6]:",
        ["7", "8", "9", "10", "11", "12"],
        [
            (PURPLE, "  1. SF7  (fastest, shortest range)"),
            (PURPLE, "  2. SF8"),
            (PURPLE, "  3. SF9"),
            (PURPLE, "  4. SF10"),
            (PURPLE, "  5. SF11"),
            (PURPLE, "  6. SF12 (slowest, longest range)"),
        ],
    )

    print_color(BLUE, "Coding Rate:")
    coding_rate = ensure_choice(
        "Select coding rate [1-4]:",
        ["5", "6", "7", "8"],
        [
            (PURPLE, "  1. 4/5 (fastest)"),
            (PURPLE, "  2. 4/6"),
            (PURPLE, "  3. 4/7"),
            (PURPLE, "  4. 4/8 (most robust)"),
        ],
    )
    power = ensure_int("Enter TX power [2-22]: ", 2, 22)
    preamble = ensure_int("Enter preamble length [8-65535]: ", 8, 65535)
    return [bandwidth, spreading_factor, coding_rate, str(power), str(preamble)]


def collect_hops() -> int:
    print_color(BLUE, "Max mesh hops (1-10):")
    return ensure_int("Enter hops: ", 1, 10)


def collect_channel() -> str:
    while True:
        name = input("Enter channel name (3-20 chars, A-Z/0-9/_): ").strip().upper()
        try:
            validate_channel(name)
            return name
        except FlashToolError as exc:
            print_color(RED, str(exc))


def collect_password(channel: str) -> str:
    while True:
        entry = input("Enter password (Enter = auto-generate): ").strip().upper()
        if not entry:
            generated = generate_password()
            print_color(GREEN, f"Password auto-generated: {generated}")
            return generated
        try:
            validate_password(channel, entry)
            return entry
        except FlashToolError as exc:
            print_color(RED, str(exc))


def interactive_configuration() -> FlashConfig:
    """Collect configuration interactively mirroring the macOS script flow."""
    config = FlashConfig()
    config.role = collect_role()
    config.device_id = collect_device_id()
    config.interval = collect_interval()
    config.region = collect_region()
    config.mode = collect_mode()
    config.radio = collect_radio(config)
    config.hops = collect_hops()
    config.channel = collect_channel()
    config.password = collect_password(config.channel)
    return config


def prompt_configuration_workflow() -> str:
    while True:
        print_color(BLUE, "Configuration Type:")
        print_color(PURPLE, "  1. Normal     - Guided step-by-step setup")
        print_color(PURPLE, "  2. One Line   - Paste a single command line")
        choice = input("Select configuration type [1-2]: ").strip()
        if choice == "1":
            return "interactive"
        if choice == "2":
            return "command"
        print_color(RED, "Invalid selection. Enter 1 or 2.")


def collect_oneline_configuration(command_parser: argparse.ArgumentParser) -> FlashConfig:
    import shlex

    print_color(BLUE, "One-line Configuration Mode")
    print_color(YELLOW, "Example:")
    print_color(GREEN, "  -role TRACKER -id 1 -interval 60 -region US -mode SIMPLE -radio DESERT_LONG_FAST -channel TEAM_ALPHA -password Secure123 -hops 3")
    print_color(YELLOW, "Available parameters: -role, -id, -interval, -region, -mode, -radio, -channel, -password, -hops, -port, -custom")

    while True:
        line = input("Enter your configuration command: ").strip()
        if not line:
            print_color(RED, "Configuration command cannot be empty.")
            continue
        try:
            tokens = shlex.split(line)
        except ValueError as exc:
            print_color(RED, f"Could not parse line: {exc}")
            continue
        try:
            namespace = command_parser.parse_args(tokens)
        except SystemExit:
            print_color(RED, "Invalid parameters. Please try again.")
            continue
        return parse_command_mode(namespace)


def compute_network_hash(channel: str, password: str) -> str:
    combined = (channel + password).upper()
    hash_value = 0
    for char in combined:
        hash_value = (hash_value * 31 + ord(char)) & 0xFFFFFFFF
    return f"{hash_value:08X}"


# ---------------------------------------------------------------------------
# Command-line parsing
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Custodia Flash Tool (Python port)",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--workflow",
        choices=["interactive", "command"],
        help="Select configuration workflow without prompting",
    )
    parser.add_argument("-role", "--role", dest="role")
    parser.add_argument("-id", "--id", dest="id", type=int)
    parser.add_argument("-interval", "--interval", dest="interval", type=int)
    parser.add_argument("-region", "--region", dest="region")
    parser.add_argument("-mode", "--mode", dest="mode")
    parser.add_argument("-radio", "--radio", dest="radio")
    parser.add_argument("-hops", "--hops", dest="hops", type=int, default=3)
    parser.add_argument("-channel", "--channel", dest="channel")
    parser.add_argument("-password", "--password", dest="password")
    parser.add_argument("-port", "--port", dest="port")
    parser.add_argument("-custom", "--custom", dest="custom", nargs=5, metavar=("BW", "SF", "CR", "POWER", "PREAMBLE"))
    return parser


def build_command_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-role", "--role", dest="role")
    parser.add_argument("-id", "--id", dest="id", type=int)
    parser.add_argument("-interval", "--interval", dest="interval", type=int)
    parser.add_argument("-region", "--region", dest="region")
    parser.add_argument("-mode", "--mode", dest="mode")
    parser.add_argument("-radio", "--radio", dest="radio")
    parser.add_argument("-hops", "--hops", dest="hops", type=int, default=3)
    parser.add_argument("-channel", "--channel", dest="channel")
    parser.add_argument("-password", "--password", dest="password")
    parser.add_argument("-port", "--port", dest="port")
    parser.add_argument("-custom", "--custom", dest="custom", nargs=5)
    return parser


def parse_command_mode(args: argparse.Namespace) -> FlashConfig:
    """Build a configuration instance directly from command mode arguments."""
    return FlashConfig(
        role=args.role,
        device_id=args.id,
        interval=args.interval,
        region=args.region,
        mode=args.mode,
        radio=args.radio,
        hops=args.hops,
        channel=args.channel,
        password=args.password,
        port=args.port,
        custom_params=args.custom or [],
    )


# ---------------------------------------------------------------------------
# Aggregated validation
# ---------------------------------------------------------------------------

def validate_config(config: FlashConfig) -> FlashConfig:
    if config.role is None or config.role.upper() not in VALID_ROLES:
        raise FlashToolError("Role must be one of TRACKER, RECEIVER, REPEATER.")
    config.role = config.role.upper()

    if config.device_id is None or not (1 <= config.device_id <= 255):
        raise FlashToolError("Device ID must be between 1 and 255.")

    if config.interval is None or not (5 <= config.interval <= 3600):
        raise FlashToolError("Interval must be 5-3600 seconds.")

    if config.region is None or config.region.upper() not in VALID_REGIONS:
        raise FlashToolError("Region must be US, EU, CH, AS, or JP.")
    config.region = config.region.upper()

    if config.mode is None or config.mode.upper() not in VALID_MODES:
        raise FlashToolError("Mode must be SIMPLE or ADMIN.")
    config.mode = config.mode.upper()

    if config.radio is None or config.radio.upper() not in VALID_RADIOS:
        raise FlashToolError("Radio must be one of the supported profiles.")
    config.radio = config.radio.upper()

    if not (1 <= config.hops <= 10):
        raise FlashToolError("Hops must be 1-10.")

    if config.channel is None:
        raise FlashToolError("Channel name is required.")
    validate_channel(config.channel.upper())
    config.channel = config.channel.upper()

    if config.password is None:
        config.password = generate_password()
        print_color(GREEN, f"Password auto-generated: {config.password}")
    validate_password(config.channel, config.password)
    config.password = config.password.upper()

    if config.radio == "CUSTOM_ADVANCED":
        if len(config.custom_params) != 5:
            raise FlashToolError("Custom radio requires 5 parameters (BW SF CR POWER PREAMBLE).")
        config.custom_params = [str(value).upper() for value in config.custom_params]

    config.network_hash = compute_network_hash(config.channel, config.password)

    return config


def summarize_config(config: FlashConfig) -> None:
    config.network_hash = compute_network_hash(config.channel or "", config.password or "")
    print_color(YELLOW, "\n[CONFIG] Configuration Summary:")
    print_color(BLUE, config.as_readable_summary())


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def main(argv: Optional[List[str]] = None) -> int:
    print_header()
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    command_parser = build_command_parser()

    script_dir, project_dir = get_paths()

    cli_fields = ["role", "id", "interval", "region", "mode", "radio", "channel", "password", "port", "custom"]
    provided_cli = any(getattr(args, field) is not None for field in cli_fields)

    workflow = args.workflow
    if workflow is None:
        workflow = "command" if provided_cli else prompt_configuration_workflow()

    if workflow == "interactive":
        config = interactive_configuration()
    else:
        if provided_cli:
            config = parse_command_mode(args)
        else:
            config = collect_oneline_configuration(command_parser)

    try:
        config = validate_config(config)
    except FlashToolError as exc:
        print_color(RED, f"Configuration error: {exc}")
        return 1

    summarize_config(config)

    try:
        serial_module = ensure_pyserial()
        config.pio_cmd = ensure_platformio()
        ensure_port(config, serial_module)
        verify_project_structure(project_dir)
        detect_board_type(config)
        clean_device_flash(config, project_dir)
        flash_firmware(config, project_dir)
        wait_for_device_reboot()
        configure_device(config, serial_module)
        launch_monitor(config, project_dir)
        show_final_summary(config)
    except FlashToolError as exc:
        print_color(RED, str(exc))
        return 1

    print_color(YELLOW, "Custodia Flash Tool completed successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
