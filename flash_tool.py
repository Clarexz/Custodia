import sys
import subprocess
import time
import argparse
import platform
from pathlib import Path

def check_and_install_pyserial():
    """Install pyserial if not available"""
    try:
        import serial
        import serial.tools.list_ports
        return True
    except ImportError:
        print("Installing pyserial...")
        try:
            subprocess.run([sys.executable, '-m', 'pip', 'install', 'pyserial'], 
                          check=True, capture_output=True)
            print("pyserial correctly installed")
            return True
        except subprocess.CalledProcessError:
            print("ERROR: Cannot innstal pyserial")
            print("Solution: pip install pyserial")
            return False
        except FileNotFoundError:
            print("ERROR: Python have not found PATH")
            print("Install Python from python.org & include in PATH")
            return False

def detect_port():
    """Enhanced ESP32 port detection"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    
    esp32_ports = []
    
    # Look for ESP32-specific patterns
    for port in ports:
        device_lower = port.device.lower()
        desc_lower = port.description.lower()
        manufacturer_lower = (port.manufacturer or '').lower()
        
        # Skip obviously non-ESP32 devices
        skip_patterns = ['bluetooth', 'debug-console', 'hub', 'keyboard', 'mouse']
        if any(pattern in desc_lower for pattern in skip_patterns):
            continue
        
        # Look for ESP32-specific indicators
        esp32_indicators = [
            # Common ESP32 USB-to-Serial chips
            'cp210', 'ch340', 'ch341', 'ft232', 'pl2303',
            # ESP32 specific descriptions
            'esp32', 'serial', 'uart', 'usb-serial',
            # Common port patterns
            'ttyusb', 'ttyacm', 'usbserial', 'usbmodem'
        ]
        
        is_esp32_candidate = False
        
        # Check device name patterns
        if any(pattern in device_lower for pattern in ['ttyusb', 'ttyacm', 'usbserial', 'usbmodem']):
            is_esp32_candidate = True
        
        # Check description patterns
        if any(indicator in desc_lower for indicator in esp32_indicators):
            is_esp32_candidate = True
        
        # Check manufacturer patterns
        if any(mfg in manufacturer_lower for mfg in ['silicon labs', 'ftdi', 'prolific', 'ch340']):
            is_esp32_candidate = True
        
        if is_esp32_candidate:
            esp32_ports.append(port)
    
    # If no ESP32 candidates found, show all ports for manual selection
    if not esp32_ports:
        print("No ESP32 devices detected automatically.")
        if ports:
            print("Available ports:")
            for i, port in enumerate(ports, 1):
                print(f"  {i}. {port.device} - {port.description}")
            
            try:
                choice = input("Select port manually [1-{}] or 'q' to quit: ".format(len(ports)))
                if choice.lower() == 'q':
                    return None
                selected = int(choice) - 1
                if 0 <= selected < len(ports):
                    return ports[selected].device
            except (ValueError, IndexError):
                pass
        
        print("No valid port selected")
        return None
    
    # If only one ESP32 candidate, use it
    if len(esp32_ports) == 1:
        print(f"ESP32 device detected: {esp32_ports[0].device} - {esp32_ports[0].description}")
        return esp32_ports[0].device
    
    # Multiple ESP32 candidates, let user choose
    print("Multiple ESP32 devices found:")
    for i, port in enumerate(esp32_ports, 1):
        print(f"  {i}. {port.device} - {port.description}")
    
    try:
        choice = input("Select an option [1-{}]: ".format(len(esp32_ports)))
        selected = int(choice) - 1
        if 0 <= selected < len(esp32_ports):
            return esp32_ports[selected].device
    except (ValueError, IndexError):
        pass
    
    print("No valid selection made")
    return None

def detect_board_type(port):
    """Detecta automáticamente el tipo de board conectado"""
    import serial.tools.list_ports
    
    try:
        # Buscar el puerto específico en la lista
        for port_info in serial.tools.list_ports.comports():
            if port_info.device == port:
                description = port_info.description.lower()
                manufacturer = (port_info.manufacturer or '').lower()
                
                # Solo detectar XIAO ESP32S3
        
        # Default: XIAO ESP32S3
        print(f"Detected: XIAO ESP32S3 (default)")
        return 'seeed_xiao_esp32s3'
        
    except Exception as e:
        print(f"Warning: Board detection failed ({e}), using XIAO ESP32S3")
        return 'seeed_xiao_esp32s3'

def install_dependencies():
    """Install all required dependencies automatically"""
    import subprocess
    import sys
    
    print("Checking and installing dependencies...")
    
    # List of required packages
    required_packages = ['pyserial', 'platformio']
    
    for package in required_packages:
        try:
            print(f"Checking {package}...")
            if package == 'pyserial':
                import serial
                print(f"✓ {package} already installed")
            elif package == 'platformio':
                # Check if pio command works
                result = subprocess.run(['pio', '--version'], 
                                      capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    print(f"✓ {package} already installed")
                else:
                    raise ImportError()
        except (ImportError, subprocess.TimeoutExpired, FileNotFoundError, subprocess.CalledProcessError):
            print(f"Installing {package}...")
            try:
                subprocess.run([sys.executable, '-m', 'pip', 'install', package], 
                             check=True, capture_output=True, text=True)
                print(f"✓ {package} installed successfully")
                
                # For PlatformIO on Windows, we need to refresh PATH
                if package == 'platformio' and sys.platform.startswith('win'):
                    print("Refreshing PATH for Windows...")
                    import time
                    time.sleep(3)  # Give Windows time to update
                
            except subprocess.CalledProcessError as e:
                print(f"✗ Failed to install {package}")
                print(f"Error: {e.stderr}")
                return False
    
    return True

def find_platformio():
    """Find PlatformIO installation with robust cross-platform detection"""
    import shutil
    import platform
    import sys
    import subprocess
    from pathlib import Path
    
    # Method 1: Check if pio is in PATH (most reliable)
    pio_cmd = shutil.which('pio')
    if pio_cmd:
        # Validate that it actually works
        try:
            result = subprocess.run([pio_cmd, '--version'], 
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                return pio_cmd
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
    
    # Method 2: Check common installation paths
    possible_paths = []
    
    if platform.system() == "Windows":
        # Windows-specific paths
        python_base = Path(sys.executable).parent
        possible_paths.extend([
            python_base / 'Scripts' / 'pio.exe',
            Path.home() / 'AppData' / 'Local' / 'Programs' / 'Python' / 'Scripts' / 'pio.exe',
            Path.home() / '.platformio' / 'penv' / 'Scripts' / 'pio.exe',
            Path('C:/Python39/Scripts/pio.exe'),
            Path('C:/Python310/Scripts/pio.exe'),
            Path('C:/Python311/Scripts/pio.exe'),
            Path('C:/Python312/Scripts/pio.exe'),
        ])
    else:
        # macOS/Linux paths
        possible_paths.extend([
            Path.home() / '.local' / 'bin' / 'pio',
            Path.home() / '.platformio' / 'penv' / 'bin' / 'pio',
            Path('/usr/local/bin/pio'),  # FIX: Now properly wrapped in Path()
        ])
    
    # Test each possible path
    for pio_path in possible_paths:
        try:
            if pio_path.exists():
                # Validate that it actually works
                result = subprocess.run([str(pio_path), '--version'], 
                                      capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    return str(pio_path)
        except (OSError, PermissionError, subprocess.TimeoutExpired, FileNotFoundError):
            continue
    
    return None

def check_and_install_platformio():
    """Check for PlatformIO and install if not found - ROBUST VERSION"""
    import subprocess
    import sys
    import time
    
    print("Checking PlatformIO installation...")
    
    # First check if already working
    pio_cmd = find_platformio()
    if pio_cmd:
        print(f"✓ PlatformIO found and working: {pio_cmd}")
        return pio_cmd
    
    print("PlatformIO not found or not working. Installing automatically...")
    
    try:
        # Install PlatformIO
        result = subprocess.run([sys.executable, '-m', 'pip', 'install', 'platformio'], 
                              check=True, capture_output=True, text=True)
        print("✓ PlatformIO installation completed")
        
        # Give time for installation to settle
        time.sleep(3)
        
        # Check if it's working now
        pio_cmd = find_platformio()
        if pio_cmd:
            print(f"✓ PlatformIO ready: {pio_cmd}")
            return pio_cmd
        
        # If still not found, try to help the user
        print("⚠ Installation completed but PlatformIO not immediately available")
        print("This is common on Windows. Trying alternative methods...")
        
        # On Windows, try refreshing PATH
        if sys.platform.startswith('win'):
            print("Refreshing environment variables...")
            time.sleep(2)
            pio_cmd = find_platformio()
            if pio_cmd:
                print(f"✓ PlatformIO now available: {pio_cmd}")
                return pio_cmd
        
        # Last resort: provide clear instructions
        print("Automatic setup incomplete. Manual steps required:")
        print()
        print("SOLUTION:")
        print("1. Close this terminal/command prompt")
        print("2. Open a NEW terminal/command prompt") 
        print("3. Test with: pio --version")
        print("4. If still not working:")
        if sys.platform.startswith('win'):
            print("   - Add Python Scripts folder to PATH")
            print("   - Or reinstall Python with 'Add to PATH' checked")
        else:
            print("   - Try: export PATH=$PATH:~/.local/bin")
            print("   - Or add ~/.local/bin to your shell profile")
        print("5. Re-run the flash tool")
        print()
        return None
            
    except subprocess.CalledProcessError as e:
        print(f"Failed to install PlatformIO")
        print(f"Error details: {e.stderr}")
        print()
        print("MANUAL INSTALLATION:")
        print("1. pip install platformio")  
        print("2. Or download from: https://platformio.org/")
        print("3. Ensure PlatformIO is in your system PATH")
        print("4. Test with: pio --version")
        return None
    except Exception as e:
        print(f"Unexpected error during installation: {e}")
        return None

def validate_parameters(args):
    """Validate all parameters including new network parameters"""
    errors = []
    
    # Validate role
    valid_roles = ['TRACKER', 'RECEIVER', 'REPEATER']
    if args.role.upper() not in valid_roles:
        errors.append(f"Role invalid: '{args.role}'. Valid values: {', '.join(valid_roles)}")
    
    # Validate ID
    if not (1 <= args.id <= 255):
        errors.append(f"ID invalid: '{args.id}'. Valid range: 1-255")
    
    # Validate transmission interval (GPS interval in seconds)
    if not (5 <= args.interval <= 3600):
        errors.append(f"Transmission interval invalid: '{args.interval}'. Valid range: 5-3600 seconds")
    
    # Validate region
    valid_regions = ['US', 'EU', 'CH', 'AS', 'JP']
    if args.region.upper() not in valid_regions:
        errors.append(f"Region invalid: '{args.region}'. Valid values: {', '.join(valid_regions)}")
        
    # Validate mode
    valid_modes = ['SIMPLE', 'ADMIN']
    if args.mode.upper() not in valid_modes:
        errors.append(f"Mode invalid: '{args.mode}'. Valid values: {', '.join(valid_modes)}")
    
    # Validate radio profile
    valid_radios = ['DESERT_LONG_FAST', 'MOUNTAIN_STABLE', 'URBAN_DENSE', 'MESH_MAX_NODES', 'CUSTOM_ADVANCED']
    if args.radio.upper() not in valid_radios:
        errors.append(f"Radio profile invalid: '{args.radio}'. Valid values: {', '.join(valid_radios)}")
    
    # Validate hops
    if not (1 <= int(args.hops) <= 10):
        errors.append(f"Hops invalid: '{args.hops}'. Valid range: 1-10")
    
    # Validate channel name (same rules as device validation)
    channel = args.channel.upper()
    if not (3 <= len(channel) <= 20):
        errors.append(f"Channel name invalid: '{args.channel}'. Must be 3-20 characters")
    
    if not channel.replace('_', '').isalnum():
        errors.append(f"Channel name invalid: '{args.channel}'. Only letters, numbers, and underscore allowed")
    
    # Check for reserved names
    reserved_names = ['CONFIG', 'ADMIN', 'DEBUG', 'SYSTEM', 'DEVICE', 'LORA', 'MESH', 'TEST', 'DEFAULT']
    if channel in reserved_names:
        errors.append(f"Channel name invalid: '{args.channel}'. Reserved name not allowed")
    
    # Validate password if provided
    if args.password:
        password = args.password.upper()
        if not (8 <= len(password) <= 32):
            errors.append(f"Password invalid: Must be 8-32 characters")
        
        has_number = any(c.isdigit() for c in password)
        has_letter = any(c.isalpha() for c in password)
        if not (has_number and has_letter):
            errors.append(f"Password insecure: Must contain at least 1 number and 1 letter")
        
        if password == channel:
            errors.append(f"Password invalid: Cannot be the same as channel name")
    
    return errors

def integrated_serial_monitor(port):
    """Launch monitor in separate window/terminal"""
    import subprocess
    import platform
    
    print(f"\n[MONITOR] Launching monitor in separate window...")
    
    try:
        # Detect operating system and launch appropriate terminal
        system = platform.system().lower()
        
        cmd = None  # Initialize cmd variable
        
        if system == "darwin":  # macOS
            # Use Terminal.app to open new window
            cmd = [
                'osascript', '-e', 
                f'tell application "Terminal" to do script "pio device monitor --baud 115200 --port {port}"'
            ]
        elif system == "windows":  # Windows
            # Use cmd.exe in new window
            cmd = [
                'cmd', '/c', 'start', 'cmd', '/k', 
                f'pio device monitor --baud 115200 --port {port}'
            ]
        elif system == "linux":  # Linux
            # Try common terminal emulators
            terminals = ['gnome-terminal', 'xterm', 'konsole', 'terminator']
            cmd = None
            for terminal in terminals:
                try:
                    subprocess.run(['which', terminal], check=True, capture_output=True)
                    if terminal == 'gnome-terminal':
                        cmd = ['gnome-terminal', '--', 'pio', 'device', 'monitor', '--baud', '115200', '--port', port]
                    elif terminal == 'xterm':
                        cmd = ['xterm', '-e', f'pio device monitor --baud 115200 --port {port}']
                    break
                except subprocess.CalledProcessError:
                    continue
        
        if cmd:
            subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print("Monitor window opened successfully")
            print("Check your taskbar/dock for the new terminal window")
        else:
            print("Could not auto-launch monitor window")
            print("Manual command:")
            print(f"pio device monitor --baud 115200 --port {port}")
            
    except Exception as e:
        print(f"Could not launch monitor window: {e}")
        print("Manual command:")
        print(f"pio device monitor --baud 115200 --port {port}")

    print("\n---------------------------------------------")
    print("DEVICE CONFIGURATION COMPLETED")
    print("Monitor launched in separate window")
    print("Status: READY FOR OPERATION")
    print("---------------------------------------------")

def main():
    # Agregar al inicio
    if not install_dependencies():
        print("System setup failed")
        sys.exit(1)
            
    print("Custodia Flash Tool")
    print("===================")
    
    # Check Python and install pyserial
    if not check_and_install_pyserial():
        sys.exit(1)
    
    # Now we can import serial after installation
    import serial
    
    # Parse arguments
    parser = argparse.ArgumentParser(description='Flash and configure ESP32-S3')
    parser.add_argument('-role', required=True, help='Device role')
    parser.add_argument('-id', type=int, required=True, help='Device ID')
    parser.add_argument('-interval', type=int, required=True, help='Transmission interval in seconds') 
    parser.add_argument('-region', required=True, help='LoRa region')
    parser.add_argument('-mode', required=True, help='Operation mode (SIMPLE/ADMIN)')
    parser.add_argument('-radio', required=True, help='Radio profile (DESERT_LONG_FAST/MOUNTAIN_STABLE/URBAN_DENSE/MESH_MAX_NODES/CUSTOM_ADVANCED)')
    parser.add_argument('-hops', type=int, default=3, help='Max hops (default: 3)')
    parser.add_argument('-channel', required=True, help='Network channel name')
    parser.add_argument('-password', help='Network password (auto-generated if not provided)')
    parser.add_argument('-port', help='COM port override')
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)
    
    args = parser.parse_args()
    
    # Auto-generate password if not provided
    import random
    import string
    if not args.password:
        # Generate 8-character password with at least 1 number and 1 letter
        chars = string.ascii_uppercase + string.digits
        password = ''
        # Ensure at least one letter and one number
        password += random.choice(string.ascii_uppercase)
        password += random.choice(string.digits)
        # Fill remaining 6 characters
        for _ in range(6):
            password += random.choice(chars)
        # Shuffle the password
        password_list = list(password)
        random.shuffle(password_list)
        args.password = ''.join(password_list)
        print(f"Password auto-generated: {args.password}")
    
    # Validate all parameters
    print("\n[VALIDATION] Validating configuration parameters...")
    errors = validate_parameters(args)
    
    if errors:
        print("ERRORS FOUND:\n")
        for error in errors:
            print(f"  {error}")
        print("\nVALID EXAMPLES:")
        print("Channel: TEAM_ALPHA, BASE_01, MOUNTAIN_OPS")
        print("Password: Secure123, MyPass99, Field2024")
        print("\nConfiguration cancelled - Fix errors and try again")
        sys.exit(1)
    
    # Display configuration
    print(f"Role: {args.role}")
    print(f"ID: {args.id}")
    print(f"Transmission interval: {args.interval} seconds")
    print(f"Region: {args.region}")
    print(f"Mode: {args.mode}")
    print(f"Radio: {args.radio}")
    print(f"Hops: {args.hops}")
    print(f"Channel: {args.channel}")
    print(f"Password: {args.password}")
    
    # Calculate network hash (same algorithm as device)
    combined = args.channel.upper() + args.password.upper()
    network_hash = 0
    for char in combined:
        network_hash = (network_hash * 31 + ord(char)) & 0xFFFFFFFF
    print(f"Network hash: {network_hash:08X}")
    
    # Check and install PlatformIO if needed
    pio_cmd = check_and_install_platformio()
    if not pio_cmd:
        print("ERROR: Could not install or find PlatformIO")
        print("Manual installation required:")
        print("1. pip install platformio")
        print("2. Or download from: https://platformio.org/")
        print("3. Add to PATH if needed")
        sys.exit(1)
    
    print(f"Usando PlatformIO: {pio_cmd}")
    
    # Step 1: Flash firmware (back to working version)
    print("\n[1/3] Flashing firmware...")

    print("Detecting ports...")
    port = detect_port()

    if not port:
        print("Flash cancelled - No valid port selected")
        sys.exit(1)

    print(f"Using port: {port}")
    
    try:
        # Detectar tipo de board automáticamente
        board_env = detect_board_type(port)
        print(f"Using environment: {board_env}")

        result = subprocess.run([pio_cmd, 'run', '-e', board_env, '--target', 'upload'], 
                            check=False, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("Firmware flashed successfully")
        else:
            print("ERROR: Fallo el flasheo")
            if result.stderr:
                print("Error details:", result.stderr[-200:])
            print("Solucion manual:")
            print("1. Cerrar monitor serial si esta abierto")
            print("2. Mantener presionado BOOT")
            print("3. Presionar y soltar RESET")
            print("4. Soltar BOOT")
            print(f"5. Ejecutar: {pio_cmd} run --target upload")
            sys.exit(1)
            
    except Exception as e:
        print(f"ERROR ejecutando PlatformIO: {e}")
        sys.exit(1)
    
    # Step 2: Configure device
    print("\n[2/3] Applying configuration...")
    time.sleep(8)  # Wait for device reboot
    
    # Re-detect port after flash (may have changed)
    port = detect_port()
    if not port:
        print("WARNING: No port detected after flash")
        print("Manual configuration required:")
        print(f"1. Connect to device manually")
        print(f"2. Send: NETWORK_CREATE {args.channel.upper()} {args.password.upper()}")
        print(f"3. Send: Q_CONFIG {args.role.upper()},{args.id},{args.interval},{args.region.upper()},{args.mode.upper()},{args.radio.upper()},{args.hops}")
        print("4. Send: CONFIG_SAVE")
        print("5. Send: START")
        sys.exit(1)

    print(f"Using configuration port: {port}")
    
    # Build configuration commands
    config_cmd_q = f"Q_CONFIG {args.role.upper()},{args.id},{args.interval},{args.region.upper()},{args.mode.upper()},{args.radio.upper()},{args.hops}"
    config_cmd_alt = f"CONFIG {args.role.upper()},{args.id},{args.interval},{args.region.upper()},{args.mode.upper()},{args.radio.upper()},{args.hops}"
    network_cmd = f"NETWORK_CREATE {args.channel.upper()} {args.password.upper()}"
    
    # Debug output - show what commands will be sent
    print(f"DEBUG: Commands to be sent:")
    print(f"  Network: {network_cmd}")
    print(f"  Config: {config_cmd_q}")
    print(f"  Role value: '{args.role.upper()}'")
    
    # Send configuration with proper timing and error handling
    try:
        ser = serial.Serial(port, 115200, timeout=15)
        time.sleep(5)  # Extended wait for device initialization
        
        print("Clearing input buffer...")
        ser.reset_input_buffer()  # Clear any startup messages
        
        # Send a test command first to ensure device is responsive
        print("Testing device communication...")
        ser.write(b'\r\n')  # Wake up device
        time.sleep(1)
        ser.write(b'STATUS\r\n')
        time.sleep(2)
        
        # Read and discard status response
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        print("Device communication established")
        
        # Step 1: Create network FIRST
        print(f"Creating network: {args.channel}")
        ser.reset_input_buffer()
        ser.write((network_cmd + '\r\n').encode())
        time.sleep(4)  # Give more time for network creation
        
        # Check network creation response
        network_response = ""
        while ser.in_waiting > 0:
            network_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        print(f"Network creation response: {network_response[:200]}...")  # Debug info
        
        # Step 2: Configure device parameters
        print(f"Configuring device parameters...")
        ser.reset_input_buffer()
        ser.write((config_cmd_q + '\r\n').encode())
        time.sleep(4)  # Wait for response
        
        # Read configuration response
        config_response = ""
        while ser.in_waiting > 0:
            config_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        print(f"Configuration response: {config_response[:200]}...")  # Debug info
        
        # If Q_CONFIG failed, try alternative
        if ('comando desconocido' in config_response.lower() or 
            'unknown command' in config_response.lower() or
            len(config_response.strip()) < 10):  # Very short response indicates failure
            
            print("Q_CONFIG not recognized, trying alternative CONFIG command...")
            ser.reset_input_buffer()
            ser.write((config_cmd_alt + '\r\n').encode())
            time.sleep(4)
            
            # Read alternative response
            while ser.in_waiting > 0:
                config_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)
        
        # Step 3: Save configuration
        print("Saving configuration to EEPROM...")
        ser.reset_input_buffer()
        ser.write(b'CONFIG_SAVE\r\n')
        time.sleep(3)
        
        # Read save response
        save_response = ""
        while ser.in_waiting > 0:
            save_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        print(f"Save response: {save_response[:100]}...")
        
        # Step 4: Start device
        print("Starting device operation...")
        ser.reset_input_buffer()
        ser.write(b'START\r\n')
        time.sleep(2)
        
        # Final status check
        ser.write(b'NETWORK_STATUS\r\n')
        time.sleep(2)
        
        final_response = ""
        while ser.in_waiting > 0:
            final_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        ser.close()
        
        print("\n=== CONFIGURATION RESULTS ===")
        
        # Check for success indicators
        success_indicators = [
            'network creada exitosamente',
            'configuracion guardada exitosamente', 
            'configuración guardada',
            'network activa',
            'listo y operando'
        ]
        
        all_responses = (network_response + config_response + save_response + final_response).lower()
        
        if any(indicator in all_responses for indicator in success_indicators):
            print("Configuration completed successfully")
            print(f"Device ID: {args.id}")
            print(f"Role: {args.role}")
            print(f"Network: {args.channel}")
            print(f"Password: {args.password}")
        else:
            print("Configuration may have failed")
            print("Manual verification required:")
            print(f"1. Connect to device: pio device monitor --port {port} --baud 115200")
            print("2. Check status: STATUS")
            print("3. Check network: NETWORK_STATUS")
            print("4. If needed, configure manually:")
            print(f"   - {network_cmd}")
            print(f"   - {config_cmd_q}")
            print("   - CONFIG_SAVE")
            print("   - START")
        
        # Launch integrated monitor
        integrated_serial_monitor(port)
            
    except Exception as e:
        print(f"CONFIGURATION ERROR: {e}")
        print("\n=== MANUAL RECOVERY ===")
        print(f"1. Connect manually: pio device monitor --port {port} --baud 115200")
        print("2. Send these commands one by one:")
        print(f"   STATUS")
        print(f"   {network_cmd}")
        print(f"   {config_cmd_q}")
        print(f"   CONFIG_SAVE")
        print(f"   START")
        print(f"   NETWORK_STATUS")
        print("\n3. Expected responses:")
        print("   - 'Network ... creada exitosamente'")
        print("   - 'Configuración guardada exitosamente'")
        print("   - 'Network activa: [YOUR_CHANNEL]'")
        sys.exit(1)
if __name__ == "__main__":
    main()