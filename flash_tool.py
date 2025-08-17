import sys
import time
import subprocess
import importlib.util
import argparse

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

def check_and_install_platformio():
    """Check for PlatformIO and install if not found"""
    print("Checking PlatformIO installation...")
    
    # First check if already installed
    pio_cmd = find_platformio()
    if pio_cmd:
        print(f"PlatformIO found: {pio_cmd}")
        return pio_cmd
    
    # Not found, try to install
    print("PlatformIO not found. Installing automatically...")
    try:
        subprocess.run([sys.executable, '-m', 'pip', 'install', 'platformio'], 
                      check=True, capture_output=True)
        print("PlatformIO installed successfully")
        
        # Check again after installation
        time.sleep(2)  # Give it a moment
        pio_cmd = find_platformio()
        if pio_cmd:
            print(f"PlatformIO ready: {pio_cmd}")
            return pio_cmd
        else:
            print("Installation completed but PlatformIO not found in PATH")
            print("Trying common post-install locations...")
            # Try to find in fresh install locations
            import os
            possible_new_paths = [
                os.path.expanduser('~/.local/bin/pio'),
                '/usr/local/bin/pio'
            ]
            for path in possible_new_paths:
                if os.path.exists(path):
                    print(f"Found PlatformIO at: {path}")
                    return path
            
            print("Manual setup may be required")
            return None
            
    except subprocess.CalledProcessError as e:
        print(f"Failed to install PlatformIO: {e}")
        return None
    except Exception as e:
        print(f"Error during installation: {e}")
        return None

def find_platformio():
    """Find PlatformIO installation"""
    import shutil
    from pathlib import Path
    
    # Check if pio is in PATH
    if shutil.which('pio'):
        return 'pio'
    
    # Check common user installation paths
    possible_paths = [
        Path.home() / '.platformio' / 'penv' / 'bin' / 'pio',
        Path.home() / '.platformio' / 'penv' / 'Scripts' / 'pio.exe',  # Windows
        Path.home() / '.local' / 'bin' / 'pio',
        '/usr/local/bin/pio',
    ]
    
    for pio_path in possible_paths:
        if pio_path.exists():
            return str(pio_path)
    
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
        # Back to the original working command - let PlatformIO auto-detect
        result = subprocess.run([pio_cmd, 'run', '--target', 'upload'], 
                               check=False, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("Firmware flasheado correctamente")
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
        print(f"2. Send: {network_cmd}")
        print(f"3. Send: {config_cmd_q}")
        print("4. Send: CONFIG_SAVE")
        print("5. Send: START")
        sys.exit(1)

    
    print(f"Usando puerto para configuracion: {port}")
    # Build configuration command - try both variants
    config_cmd_q = f"Q_CONFIG {args.role},{args.id},{args.interval},{args.region},{args.mode},{args.radio},{args.hops}"
    config_cmd_alt = f"CONFIG {args.role},{args.id},{args.interval},{args.region},{args.mode},{args.radio},{args.hops}"
    
    network_cmd = f"NETWORK_CREATE {args.channel.upper()} {args.password.upper()}"
    
    # Send configuration
    try:
        ser = serial.Serial(port, 115200, timeout=10)
        time.sleep(3)  # Wait for device initialization
        
        ser.write((network_cmd + '\r\n').encode())  # ← AGREGAR ESTA LÍNEA
        time.sleep(2)  # Wait for network creation 
        
        # Try Q_CONFIG first
        print(f"Configuring device...")
        ser.write((config_cmd_q + '\r\n').encode())
        time.sleep(3)  # Wait for response
        
        # Read response
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        config_success = any(word in response.lower() for word in ['completada exitosamente', 'configuracion guardada', '[ok]'])
        
        if config_success and args.channel and args.channel != 'default':
            print(f"\n[3/3] Configurando canal de seguridad: {args.channel}")
            
            # LÓGICA CORREGIDA: Determinar comando según si se proporciona PSK
            if args.psk:
                channel_cmd = f"NETWORK_CREATE {args.channel} PSK {args.psk}"
                print(f"Usando PSK proporcionado para canal '{args.channel}'")
                print(f"[DEBUG] Comando NETWORK_CREATE: '{channel_cmd}'")
            else:
                channel_cmd = f"NETWORK_CREATE {args.channel}"
                print(f"Generando PSK aleatorio para canal '{args.channel}'")
                print(f"[DEBUG] Comando NETWORK_CREATE: '{channel_cmd}'")
            
            
            print(f"Enviando: {channel_cmd}")
            ser.write((channel_cmd + '\r\n').encode())
            time.sleep(2)  # Wait for channel creation
            
            # Read channel creation response
            channel_response = ""
            while ser.in_waiting > 0:
                channel_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)
            
            # DETECCIÓN MEJORADA: Más patterns de éxito
            success_patterns = [
                "[OK]", "creado exitosamente", "Canal", "creado", 
                "exitosamente", "OK", "created", "success", "configurado"
            ]
            
            channel_success = any(pattern.lower() in channel_response.lower() 
                                 for pattern in success_patterns)
            
            # También considerar éxito si hay respuesta substantiva
            has_substantial_response = len(channel_response.strip()) > 10
            
            if channel_success or has_substantial_response:
                print(f"Canal '{args.channel}' configurado")
                print("Canal guardado automáticamente en EEPROM")
                
                # FUNCIONALIDAD PRINCIPAL: Si NO se proporcionó PSK, mostrar opción
                if not args.psk:
                    show_psk_copy_option(ser, args.channel)
            else:
                print(f"Respuesta inesperada del canal {args.channel}")
                print(f"Respuesta: '{channel_response}'")
                print("Verificar manualmente con: config> NETWORK_LIST")
                
                # Aún así, intentar mostrar PSK si no se proporcionó
                if not args.psk:
                    print("\n¿Intentar obtener PSK de todas formas? (Y/N): ", end="")
                    try_anyway = input().strip().upper()
                    if try_anyway in ['Y', 'YES', 'S', 'SI']:
                        show_psk_copy_option(ser, args.channel)
            
            # Send START command to begin operation
            print("Iniciando dispositivo...")
            ser.write(b'START\r\n')
            time.sleep(2)
            
            # Read final response
            start_response = ""
            while ser.in_waiting > 0:
                start_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)
                
        # TERCERO: Guardar configuración
        print("Saving configuration...")
        ser.write(('CONFIG_SAVE' + '\r\n').encode())
        time.sleep(2)
        
        # CUARTO: Iniciar sistema
        print("Starting device...")
        ser.write(('START' + '\r\n').encode())
        time.sleep(2)
        
        print("\n[3/3]Configuration completed successfully")
        
        ser.close()
        
        # Check if configuration was successful
        if any(word in response.lower() for word in ['completada exitosamente', 'configuracion guardada', 'listo y operando']):
            print("Device ready to use")
        
        # Launch integrated monitor
        integrated_serial_monitor(port)
            
    except Exception as e:
        print(f"ERROR: {e}")
        print("Manual verification:")
        print(f"1. pio device monitor --baud 115200 --port {port}")
        print("2. Execute: STATUS")
        print(f"3. If not configured, send manually:")
        print(f"   {network_cmd}")        # ← AGREGAR ESTA LÍNEA
        print(f"   {config_cmd_q}")
        print(f"   O alternativamente: {config_cmd_alt}")
        print("4. Then: CONFIG_SAVE")
        print("5. Finally: START")

if __name__ == "__main__":
    main()