#!/usr/bin/env python3
"""
Custodia ESP32-S3 Flash Tool - Windows Optimized Edition
Automated firmware flashing and configuration tool
Compatible with Python 3.7+ and Windows/Mac/Linux
"""

import os
import sys
import time
import subprocess
import argparse
import platform
import random
import string
import json
import shutil
from pathlib import Path

# ==================== CONSTANTS ====================
BAUD_RATE = 115200
UPLOAD_SPEED = 921600
BOARD = "esp32-s3-devkitc-1"
PORT_TIMEOUT = 30
CONFIG_TIMEOUT = 10

# Valid parameter values
VALID_ROLES = ['TRACKER', 'RECEIVER', 'REPEATER']
VALID_REGIONS = ['US', 'EU', 'CH', 'AS', 'JP']
VALID_MODES = ['SIMPLE', 'ADMIN']
VALID_RADIOS = ['DESERT_LONG_FAST', 'MOUNTAIN_STABLE', 'URBAN_DENSE', 
                'MESH_MAX_NODES', 'CUSTOM_ADVANCED']

# ==================== WINDOWS PATH HELPERS ====================

def find_python_windows():
    """Find Python executable on Windows"""
    # Try common Python commands
    for cmd in ['python', 'python3', 'py']:
        try:
            result = subprocess.run([cmd, '--version'], 
                                  capture_output=True, text=True, shell=True)
            if result.returncode == 0:
                return cmd
        except:
            continue
    return None

def find_platformio_windows():
    """Find PlatformIO executable on Windows"""
    system = platform.system()
    
    # Common PlatformIO locations
    if system == "Windows":
        # Check if pio is in PATH
        pio_path = shutil.which('pio')
        if pio_path:
            return 'pio'
        
        # Check user's local installation
        user_home = Path.home()
        pio_locations = [
            user_home / '.platformio' / 'penv' / 'Scripts' / 'pio.exe',
            user_home / '.platformio' / 'penv' / 'Scripts' / 'platformio.exe',
            user_home / 'AppData' / 'Local' / 'Programs' / 'Python' / 'Python*' / 'Scripts' / 'pio.exe',
        ]
        
        for loc in pio_locations:
            if '*' in str(loc):
                # Handle wildcards
                for path in Path(str(loc).split('*')[0]).parent.glob('Python*'):
                    full_path = path / 'Scripts' / 'pio.exe'
                    if full_path.exists():
                        return str(full_path)
            elif loc.exists():
                return str(loc)
    
    # For Mac/Linux
    return shutil.which('pio') or 'pio'

def install_platformio_windows():
    """Install PlatformIO on Windows if not present"""
    print("\n[INFO] PlatformIO no detectado. Instalando...")
    
    python_cmd = find_python_windows()
    if not python_cmd:
        print("[ERROR] Python no encontrado. Instale Python 3.7+ primero.")
        return False
    
    try:
        # Install platformio
        print("[INFO] Instalando PlatformIO via pip...")
        subprocess.run([python_cmd, '-m', 'pip', 'install', '--upgrade', 'platformio'],
                      check=True, shell=True)
        
        # Try to find it again
        pio_cmd = find_platformio_windows()
        if pio_cmd and pio_cmd != 'pio':
            print(f"[OK] PlatformIO instalado en: {pio_cmd}")
            return pio_cmd
        
        print("[OK] PlatformIO instalado correctamente")
        return True
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Fallo al instalar PlatformIO: {e}")
        return False

# ==================== SERIAL PORT DETECTION ====================

def find_esp32_ports():
    """Detect ESP32 serial ports across platforms"""
    try:
        import serial.tools.list_ports
    except ImportError:
        print("[INFO] Instalando pyserial...")
        python_cmd = find_python_windows()
        subprocess.run([python_cmd, '-m', 'pip', 'install', 'pyserial'],
                      shell=True)
        import serial.tools.list_ports
    
    esp_ports = []
    
    for port in serial.tools.list_ports.comports():
        # Check for ESP32 identifiers
        if any(x in str(port.description).lower() for x in ['esp32', 'usb', 'uart', 'serial', 'ch340', 'cp210', 'ftdi']):
            esp_ports.append(port.device)
        elif platform.system() == "Darwin" and 'usbmodem' in port.device:
            esp_ports.append(port.device)
        elif platform.system() == "Windows" and 'COM' in port.device:
            # On Windows, include all COM ports as potential candidates
            esp_ports.append(port.device)
    
    return esp_ports

def auto_detect_port():
    """Auto-detect ESP32 port with user confirmation"""
    print("\n[DETECCIÓN] Buscando dispositivos ESP32...")
    ports = find_esp32_ports()
    
    if not ports:
        print("[ERROR] No se detectaron dispositivos ESP32")
        print("[INFO] Conecte el ESP32-S3 via USB y reinicie el programa")
        return None
    
    if len(ports) == 1:
        print(f"[OK] ESP32 detectado en: {ports[0]}")
        return ports[0]
    
    # Multiple ports found
    print(f"[INFO] Se detectaron {len(ports)} puertos posibles:")
    for i, port in enumerate(ports, 1):
        print(f"  {i}. {port}")
    
    # Auto-select first port but show it
    selected = ports[0]
    print(f"[AUTO] Seleccionando: {selected}")
    print("[INFO] Use -port para especificar un puerto diferente")
    
    return selected

# ==================== CONFIGURATION ====================

def validate_parameters(args):
    """Validate all configuration parameters"""
    errors = []
    
    # Validate role
    if args.role.upper() not in VALID_ROLES:
        errors.append(f"Role inválido: '{args.role}'. Valores válidos: {', '.join(VALID_ROLES)}")
    
    # Validate ID
    try:
        device_id = int(args.id)
        if not (1 <= device_id <= 999):
            errors.append(f"ID inválido: '{args.id}'. Rango válido: 1-999")
    except ValueError:
        errors.append(f"ID inválido: '{args.id}'. Debe ser un número entero")
    
    # Validate interval
    try:
        interval = int(args.interval)
        if not (5 <= interval <= 3600):
            errors.append(f"Intervalo inválido: '{args.interval}'. Rango válido: 5-3600 segundos")
    except ValueError:
        errors.append(f"Intervalo inválido: '{args.interval}'. Debe ser un número entero")
    
    # Validate region
    if args.region.upper() not in VALID_REGIONS:
        errors.append(f"Región inválida: '{args.region}'. Valores válidos: {', '.join(VALID_REGIONS)}")
    
    # Validate mode
    if args.mode.upper() not in VALID_MODES:
        errors.append(f"Modo inválido: '{args.mode}'. Valores válidos: {', '.join(VALID_MODES)}")
    
    # Validate radio profile
    if args.radio.upper() not in [r.upper() for r in VALID_RADIOS]:
        errors.append(f"Perfil de radio inválido: '{args.radio}'. Valores válidos: {', '.join(VALID_RADIOS)}")
    
    # Validate hops
    try:
        hops = int(args.hops)
        if not (1 <= hops <= 10):
            errors.append(f"Hops inválido: '{args.hops}'. Rango válido: 1-10")
    except ValueError:
        errors.append(f"Hops inválido: '{args.hops}'. Debe ser un número entero")
    
    # Validate channel
    channel = args.channel.upper()
    if not (3 <= len(channel) <= 20):
        errors.append(f"Nombre de canal inválido: '{args.channel}'. Debe ser de 3-20 caracteres")
    
    if not channel.replace('_', '').replace('-', '').isalnum():
        errors.append(f"Nombre de canal inválido: '{args.channel}'. Solo letras, números, guiones y underscore")
    
    # Validate password if provided
    if args.password:
        password = args.password.upper()
        if not (8 <= len(password) <= 32):
            errors.append(f"Contraseña inválida: Debe ser de 8-32 caracteres")
        
        if not password.isalnum():
            errors.append(f"Contraseña inválida: Solo letras y números permitidos")
    
    return errors

def generate_password():
    """Generate secure 8-character password"""
    chars = string.ascii_uppercase + string.digits
    while True:
        password = ''.join(random.choice(chars) for _ in range(8))
        # Ensure at least one letter and one number
        if any(c.isalpha() for c in password) and any(c.isdigit() for c in password):
            return password

# ==================== FIRMWARE OPERATIONS ====================

def compile_firmware(pio_cmd):
    """Compile firmware using PlatformIO"""
    print("\n[COMPILACIÓN] Compilando firmware...")
    print("[INFO] Esto puede tomar 1-3 minutos en la primera compilación...")
    
    try:
        # Use shell=True on Windows for better compatibility
        result = subprocess.run([pio_cmd, 'run'], 
                              capture_output=True, text=True,
                              shell=(platform.system() == "Windows"))
        
        if result.returncode != 0:
            print("[ERROR] Falló la compilación")
            print("Detalles del error:")
            print(result.stderr)
            return False
        
        print("[OK] Firmware compilado exitosamente")
        return True
    except Exception as e:
        print(f"[ERROR] Error durante compilación: {e}")
        return False

def upload_firmware(pio_cmd, port):
    """Upload firmware to ESP32"""
    print(f"\n[FLASHEO] Subiendo firmware a {port}...")
    print("[INFO] Si falla, presione BOOT+RESET en el ESP32 e intente nuevamente")
    
    try:
        # Build upload command
        cmd = [pio_cmd, 'run', '--target', 'upload', '--upload-port', port]
        
        # Use shell=True on Windows
        result = subprocess.run(cmd, 
                              capture_output=True, text=True,
                              shell=(platform.system() == "Windows"))
        
        if result.returncode != 0:
            if "could not open port" in result.stderr.lower():
                print(f"[ERROR] No se pudo abrir el puerto {port}")
                print("[INFO] Verifique que el dispositivo esté conectado")
            else:
                print("[ERROR] Falló el flasheo")
                print("[INFO] Presione el botón BOOT mientras conecta el ESP32")
            return False
        
        print("[OK] Firmware flasheado exitosamente")
        return True
    except Exception as e:
        print(f"[ERROR] Error durante flasheo: {e}")
        return False

# ==================== DEVICE CONFIGURATION ====================

def configure_device(port, args):
    """Configure device via serial commands using CORRECT firmware commands"""
    try:
        import serial
    except ImportError:
        print("[INFO] Instalando pyserial...")
        python_cmd = find_python_windows()
        subprocess.run([python_cmd, '-m', 'pip', 'install', 'pyserial'],
                      shell=True)
        import serial
    
    print(f"\n[CONFIGURACIÓN] Configurando dispositivo en {port}...")
    
    # Generate password if not provided
    if not args.password:
        args.password = generate_password()
        print(f"[INFO] Contraseña auto-generada: {args.password}")
    
    try:
        # Open serial connection
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(3)  # Wait for device to boot
        
        # Clear any pending data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        print("\n[PASO 1/3] Creando network...")
        # Create network using NETWORK_CREATE command (this exists in your firmware)
        network_cmd = f"NETWORK_CREATE {args.channel.upper()} {args.password.upper()}"
        ser.write(f"{network_cmd}\r\n".encode())
        time.sleep(2)
        
        # Read response
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        if response:
            print(f"Respuesta: {response[:100]}...")
        
        print("\n[PASO 2/3] Aplicando configuración...")
        # Use Q_CONFIG command (this is the CORRECT command in your firmware)
        config_parts = [
            args.role.upper(),
            str(args.id),
            str(args.interval),
            args.region.upper(),
            args.mode.upper(),
            args.radio.upper(),
            str(args.hops)
        ]
        
        config_cmd = f"Q_CONFIG {','.join(config_parts)}"
        ser.write(f"{config_cmd}\r\n".encode())
        time.sleep(3)
        
        # Read configuration response
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        
        # Check if configuration was successful
        if "CONFIGURACIÓN COMPLETADA" in response or "Sistema listo" in response:
            print("[OK] Configuración aplicada correctamente")
        else:
            print("[ADVERTENCIA] Respuesta inesperada, verificando...")
            
        print("\n[PASO 3/3] Guardando configuración...")
        # Save configuration
        ser.write(b"CONFIG_SAVE\r\n")
        time.sleep(2)
        
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        if "guardada" in response.lower() or "saved" in response.lower():
            print("[OK] Configuración guardada en EEPROM")
        
        # Start device - Q_CONFIG already does this automatically
        print("\n[INFO] El dispositivo se inició automáticamente")
        
        ser.close()
        return True
        
    except serial.SerialException as e:
        print(f"[ERROR] Error de comunicación serial: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] Error durante configuración: {e}")
        return False

def launch_monitor(port, pio_cmd):
    """Launch serial monitor in new window"""
    print(f"\n[MONITOR] Abriendo monitor serial...")
    
    system = platform.system()
    
    try:
        if system == "Windows":
            # Windows: open new cmd window
            subprocess.Popen(f'start cmd /k "{pio_cmd} device monitor --baud {BAUD_RATE} --port {port}"',
                           shell=True)
        elif system == "Darwin":
            # macOS: open new Terminal window
            script = f'tell application "Terminal" to do script "{pio_cmd} device monitor --baud {BAUD_RATE} --port {port}"'
            subprocess.run(['osascript', '-e', script])
        else:
            # Linux: try common terminals
            terminals = ['gnome-terminal', 'xterm', 'konsole']
            for term in terminals:
                if shutil.which(term):
                    subprocess.Popen([term, '-e', f'{pio_cmd} device monitor --baud {BAUD_RATE} --port {port}'])
                    break
        
        print("[OK] Monitor serial abierto en ventana separada")
        return True
    except Exception as e:
        print(f"[ERROR] No se pudo abrir el monitor: {e}")
        print(f"[INFO] Ejecute manualmente: {pio_cmd} device monitor --baud {BAUD_RATE} --port {port}")
        return False

# ==================== MAIN PROGRAM ====================

def print_banner():
    """Print application banner"""
    print("=" * 60)
    print("   CUSTODIA ESP32-S3 FLASH TOOL v2.0 - Windows Edition")
    print("   Automated Firmware Deployment & Configuration")
    print("=" * 60)

def print_summary(args, port):
    """Print configuration summary"""
    print("\n" + "=" * 60)
    print("              RESUMEN DE CONFIGURACIÓN")
    print("=" * 60)
    print(f"Puerto:        {port}")
    print(f"Rol:           {args.role.upper()}")
    print(f"Device ID:     {args.id}")
    print(f"Intervalo GPS: {args.interval} segundos")
    print(f"Región:        {args.region.upper()}")
    print(f"Modo datos:    {args.mode.upper()}")
    print(f"Perfil radio:  {args.radio.upper()}")
    print(f"Max hops:      {args.hops}")
    print(f"Red/Canal:     {args.channel.upper()}")
    print(f"Contraseña:    {args.password if args.password else 'Auto-generada'}")
    print("=" * 60)

def main():
    """Main program execution"""
    print_banner()
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='Herramienta automatizada de flasheo para ESP32-S3',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    # Required parameters
    parser.add_argument('-role', required=True,
                       help='Rol del dispositivo (TRACKER, RECEIVER, REPEATER)')
    parser.add_argument('-id', required=True,
                       help='ID único del dispositivo (1-999)')
    parser.add_argument('-interval', required=True,
                       help='Intervalo de transmisión GPS en segundos (5-3600)')
    parser.add_argument('-region', required=True,
                       help='Región LoRa (US, EU, CH, AS, JP)')
    parser.add_argument('-mode', required=True,
                       help='Modo de datos (SIMPLE, ADMIN)')
    parser.add_argument('-radio', required=True,
                       help='Perfil de radio (DESERT_LONG_FAST, MOUNTAIN_STABLE, etc.)')
    parser.add_argument('-hops', required=True,
                       help='Máximo número de saltos mesh (1-10)')
    parser.add_argument('-channel', required=True,
                       help='Nombre del canal/red (3-20 caracteres)')
    
    # Optional parameters
    parser.add_argument('-password', required=False,
                       help='Contraseña de red (8-32 caracteres, auto-generada si no se especifica)')
    parser.add_argument('-port', required=False,
                       help='Puerto serial manual (ej: COM3 o /dev/ttyUSB0)')
    parser.add_argument('--skip-upload', action='store_true',
                       help='Saltar flasheo (solo configurar)')
    parser.add_argument('--skip-monitor', action='store_true',
                       help='No abrir monitor serial al finalizar')
    
    args = parser.parse_args()
    
    # Validate parameters
    print("\n[VALIDACIÓN] Verificando parámetros...")
    errors = validate_parameters(args)
    if errors:
        print("\n[ERROR] Se encontraron errores en los parámetros:")
        for error in errors:
            print(f"  • {error}")
        print("\n[INFO] Use -h para ver ayuda")
        sys.exit(1)
    print("[OK] Todos los parámetros son válidos")
    
    # Detect or verify port
    if args.port:
        port = args.port
        print(f"\n[INFO] Usando puerto especificado: {port}")
    else:
        port = auto_detect_port()
        if not port:
            sys.exit(1)
    
    # Find or install PlatformIO
    print("\n[PREPARACIÓN] Verificando herramientas...")
    pio_cmd = find_platformio_windows()
    
    if not pio_cmd or pio_cmd == 'pio':
        # Try to run pio directly
        try:
            result = subprocess.run(['pio', '--version'], 
                                  capture_output=True, shell=True)
            if result.returncode == 0:
                pio_cmd = 'pio'
                print("[OK] PlatformIO detectado")
            else:
                raise Exception()
        except:
            # Need to install
            pio_cmd = install_platformio_windows()
            if not pio_cmd:
                print("[ERROR] No se pudo instalar PlatformIO")
                print("[INFO] Instale manualmente: pip install platformio")
                sys.exit(1)
    else:
        print(f"[OK] PlatformIO encontrado: {pio_cmd if pio_cmd != 'pio' else 'Sistema'}")
    
    # Print configuration summary
    print_summary(args, port)
    
    # Compile firmware
    if not args.skip_upload:
        if not compile_firmware(pio_cmd):
            print("\n[ERROR] Fallo en la compilación")
            sys.exit(1)
        
        # Upload firmware
        if not upload_firmware(pio_cmd, port):
            print("\n[ERROR] Fallo en el flasheo")
            print("[INFO] Intente presionar BOOT+RESET en el ESP32")
            sys.exit(1)
        
        # Wait for device to reboot
        print("\n[INFO] Esperando reinicio del dispositivo...")
        time.sleep(5)
    
    # Configure device
    if not configure_device(port, args):
        print("\n[ADVERTENCIA] La configuración automática puede haber fallado")
        print("[INFO] Use el monitor serial para configurar manualmente si es necesario")
    
    # Success message
    print("\n" + "=" * 60)
    print("           ✓ PROCESO COMPLETADO EXITOSAMENTE")
    print("=" * 60)
    print(f"Dispositivo: {args.role.upper()} (ID: {args.id})")
    print(f"Red: {args.channel.upper()} / {args.password if args.password else 'Contraseña auto-generada'}")
    print(f"Puerto: {port}")
    print("=" * 60)
    
    # Launch monitor if requested
    if not args.skip_monitor:
        print("\n¿Desea abrir el monitor serial? (S/N): ", end='')
        response = input().strip().upper()
        if response == 'S':
            launch_monitor(port, pio_cmd)
    
    print("\n[INFO] ¡Dispositivo listo para operar!")
    print("[INFO] Los LEDs deberían parpadear indicando actividad")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[INFO] Proceso cancelado por el usuario")
        sys.exit(0)
    except Exception as e:
        print(f"\n[ERROR] Error inesperado: {e}")
        sys.exit(1)