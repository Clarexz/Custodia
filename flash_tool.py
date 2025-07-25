#!/usr/bin/env python3
"""
Custom Meshtastic Flash Tool v2.1 for Mac/Linux
Auto-install dependencies and flash ESP32-S3 devices
Now with automatic PlatformIO installation!
"""

import sys
import os
import subprocess
import serial
from serial.tools import list_ports
import time
import argparse
import shutil
from pathlib import Path

# Configuration constants
BAUD_RATE = 115200
FLASH_TIMEOUT = 120  # seconds
CONFIG_TIMEOUT = 10  # seconds
VERSION = "2.1"

def print_header():
    """Print tool header"""
    print("=" * 60)
    print(f"  CUSTOM MESHTASTIC FLASH TOOL v{VERSION} (Mac/Linux)")
    print("  WITH AUTOMATIC DEPENDENCY INSTALLATION")
    print("=" * 60)
    print()

def check_python_version():
    """Check if Python version is adequate"""
    print("[0/6] Verificando versión de Python...")
    
    if sys.version_info < (3, 7):
        print(f"ERROR: Se requiere Python 3.7 o superior")
        print(f"Versión actual: {sys.version}")
        print()
        print("SOLUCIÓN:")
        print("1. Instale Python 3.7+ desde python.org")
        print("2. En Mac, puede usar: brew install python3")
        print("3. En Ubuntu: sudo apt update && sudo apt install python3 python3-pip")
        return False
    
    print(f"Python {sys.version.split()[0]} encontrado ✓")
    return True

def check_and_install_pip():
    """Ensure pip is available"""
    print("Verificando pip...")
    
    try:
        subprocess.run([sys.executable, '-m', 'pip', '--version'], 
                      check=True, capture_output=True)
        print("pip encontrado ✓")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("pip no encontrado. Intentando instalar...")
        
        # Try to install pip
        try:
            # Download get-pip.py method
            import urllib.request
            get_pip_url = "https://bootstrap.pypa.io/get-pip.py"
            get_pip_path = "/tmp/get-pip.py"
            
            print("Descargando get-pip.py...")
            urllib.request.urlretrieve(get_pip_url, get_pip_path)
            
            print("Instalando pip...")
            subprocess.run([sys.executable, get_pip_path], check=True)
            
            # Clean up
            os.remove(get_pip_path)
            print("pip instalado ✓")
            return True
            
        except Exception as e:
            print(f"ERROR: No se pudo instalar pip: {e}")
            print()
            print("SOLUCIÓN MANUAL:")
            print("Mac: brew install python3")
            print("Ubuntu: sudo apt install python3-pip")
            print("CentOS/RHEL: sudo yum install python3-pip")
            return False

def install_python_dependencies():
    """Install required Python packages"""
    print("Verificando dependencias de Python...")
    
    required_packages = ['pyserial']
    
    for package in required_packages:
        try:
            __import__(package.replace('-', '_'))
            print(f"{package} encontrado ✓")
        except ImportError:
            print(f"Instalando {package}...")
            try:
                subprocess.run([sys.executable, '-m', 'pip', 'install', package], 
                             check=True, capture_output=True)
                print(f"{package} instalado ✓")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: No se pudo instalar {package}")
                print("Intente ejecutar con permisos de administrador o usar --user")
                print(f"Comando manual: pip3 install --user {package}")
                return False
    
    return True

def check_and_install_platformio():
    """Check if PlatformIO is available, install if not"""
    print("Verificando PlatformIO Core...")
    
    # Check if pio command is available
    if shutil.which('pio'):
        print("PlatformIO encontrado ✓")
        return True
    
    # Check in common user installation paths
    possible_paths = [
        Path.home() / '.platformio' / 'penv' / 'bin' / 'pio',
        Path.home() / '.local' / 'bin' / 'pio',
        Path('/usr/local/bin/pio'),
    ]
    
    for pio_path in possible_paths:
        if pio_path.exists():
            print(f"PlatformIO encontrado en: {pio_path} ✓")
            # Add to PATH for current session
            os.environ['PATH'] = f"{pio_path.parent}:{os.environ.get('PATH', '')}"
            return True
    
    # PlatformIO not found, install it
    print("PlatformIO no encontrado. Instalando...")
    print()
    print("NOTA: Esto puede tomar 3-5 minutos y requiere conexión a internet")
    print()
    
    try:
        # Install PlatformIO
        print("Ejecutando: pip3 install platformio")
        result = subprocess.run([sys.executable, '-m', 'pip', 'install', 'platformio'], 
                               check=True, capture_output=False)
        
        print()
        print("PlatformIO instalado ✓")
        
        # Update PATH for current session
        user_bin = Path.home() / '.local' / 'bin'
        if user_bin.exists():
            os.environ['PATH'] = f"{user_bin}:{os.environ.get('PATH', '')}"
        
        # Verify installation
        time.sleep(2)
        if shutil.which('pio'):
            print("PlatformIO verificado ✓")
            return True
        else:
            print("NOTA: PlatformIO instalado pero no en PATH actual")
            print("Si el siguiente paso falla, reinicie la terminal")
            return True
            
    except subprocess.CalledProcessError as e:
        print()
        print("ERROR: Falló la instalación de PlatformIO")
        print()
        print("POSIBLES SOLUCIONES:")
        print("1. Ejecutar con permisos: sudo pip3 install platformio")
        print("2. Instalar en user space: pip3 install --user platformio")
        print("3. Usar homebrew (Mac): brew install platformio")
        print("4. Verificar conexión a internet")
        print()
        return False

def detect_esp32_port():
    """Auto-detect ESP32-S3 USB port"""
    print("[1/6] Detectando ESP32-S3...")
    
    # ESP32-S3 common USB identifiers
    esp32_vendors = [0x10c4, 0x1a86, 0x0403, 0x067b, 0x1b1c]
    esp32_products = [0xea60, 0x7523, 0x6001, 0x2303, 0x1001]
    
    ports = list_ports.comports()
    
    # First try: Look for ESP32 specific identifiers
    for port in ports:
        if port.vid and port.pid:
            if port.vid in esp32_vendors or port.pid in esp32_products:
                print(f"ESP32 detectado en: {port.device}")
                return port.device
    
    # Second try: Look for common USB-to-serial chips by description
    for port in ports:
        description = port.description.lower()
        if any(chip in description for chip in ['cp210', 'ch340', 'ch341', 'ftdi', 'silicon labs']):
            print(f"Adaptador USB-Serial encontrado: {port.device}")
            print(f"Descripción: {port.description}")
            return port.device
    
    # Third try: Look for common device paths (Mac/Linux specific)
    common_patterns = [
        '/dev/ttyUSB*',
        '/dev/ttyACM*', 
        '/dev/cu.usbserial*',
        '/dev/cu.SLAB_USBtoUART*',
        '/dev/cu.wchusbserial*'
    ]
    
    import glob
    detected_ports = []
    for pattern in common_patterns:
        detected_ports.extend(glob.glob(pattern))
    
    if detected_ports:
        if len(detected_ports) == 1:
            print(f"Puerto serial detectado: {detected_ports[0]}")
            return detected_ports[0]
        else:
            print("Múltiples puertos seriales encontrados:")
            for i, port in enumerate(detected_ports, 1):
                print(f"  {i}. {port}")
            
            while True:
                try:
                    choice = input(f"Seleccione puerto (1-{len(detected_ports)}): ").strip()
                    if choice.isdigit() and 1 <= int(choice) <= len(detected_ports):
                        selected_port = detected_ports[int(choice) - 1]
                        print(f"Seleccionado: {selected_port}")
                        return selected_port
                    else:
                        print("Selección inválida. Intente nuevamente.")
                except KeyboardInterrupt:
                    print("\nOperación cancelada.")
                    sys.exit(1)
    
    # Fourth try: Show all available ports
    if ports:
        print("Puertos serie disponibles:")
        for i, port in enumerate(ports, 1):
            print(f"  {i}. {port.device} - {port.description}")
        
        while True:
            try:
                choice = input(f"Seleccione puerto (1-{len(ports)}): ").strip()
                if choice.isdigit() and 1 <= int(choice) <= len(ports):
                    selected_port = ports[int(choice) - 1].device
                    print(f"Seleccionado: {selected_port}")
                    return selected_port
                else:
                    print("Selección inválida. Intente nuevamente.")
            except KeyboardInterrupt:
                print("\nOperación cancelada.")
                sys.exit(1)
    else:
        print("ERROR: No se encontraron puertos serie")
        print()
        print("SOLUCIÓN:")
        print("- Asegúrese de que el ESP32-S3 esté conectado vía USB")
        print("- Verifique que los drivers estén instalados")
        print("- Intente con un cable USB diferente")
        print("- En Linux, verifique permisos: sudo usermod -a -G dialout $USER")
        return None

def install_project_dependencies():
    """Install project-specific libraries"""
    print("[2/6] Instalando dependencias del proyecto...")
    print()
    print("Instalando librerías necesarias (RadioLib, etc.)")
    
    try:
        # First try platformio lib install (auto-detects from platformio.ini)
        result = subprocess.run(['pio', 'lib', 'install'], 
                              capture_output=True, text=True, timeout=60)
        
        if result.returncode == 0:
            print("Dependencias del proyecto instaladas ✓")
        else:
            print("Advertencia: Algunas dependencias pueden faltar, pero continuando...")
            print("Salida:", result.stderr)
        
        return True
        
    except subprocess.TimeoutExpired:
        print("Advertencia: Timeout instalando dependencias, pero continuando...")
        return True
    except Exception as e:
        print(f"Advertencia: Error instalando dependencias: {e}")
        print("Continuando de todas formas...")
        return True

def flash_firmware(port):
    """Flash firmware using PlatformIO"""
    print("[3/6] Flasheando firmware...")
    print()
    print("Compilando y subiendo firmware...")
    print("Esto puede tomar 1-3 minutos...")
    print()
    
    try:
        # Run PlatformIO upload command
        cmd = ['pio', 'run', '--target', 'upload', '--upload-port', port]
        
        # Show real-time output
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, 
                                 universal_newlines=True, bufsize=1)
        
        # Print output in real-time
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())
        
        return_code = process.poll()
        
        if return_code == 0:
            print()
            print("Firmware flasheado exitosamente ✓")
            print()
            return True
        else:
            print()
            print("ERROR: Falló el flash del firmware")
            print()
            print("POSIBLES SOLUCIONES:")
            print("1. Mantener presionado BOOT, presionar RESET, soltar BOOT")
            print("2. Verificar conexiones de hardware")
            print("3. Verificar que el puerto sea correcto")
            print("4. Intentar con un cable USB diferente")
            print("5. En Linux, verificar permisos del puerto serial")
            return False
            
    except subprocess.TimeoutExpired:
        print()
        print("ERROR: Timeout durante el flash")
        print("Intente el modo bootloader manual:")
        print("1. Mantener presionado botón BOOT")
        print("2. Presionar y soltar botón RESET")
        print("3. Soltar botón BOOT")
        return False
    except KeyboardInterrupt:
        print("\nOperación cancelada por usuario")
        return False
    except Exception as e:
        print(f"ERROR: Error inesperado durante flash: {e}")
        return False

def send_config_command(port, command):
    """Send configuration command via serial"""
    print("[4/6] Configurando dispositivo...")
    print()
    
    try:
        # Wait for device to restart after flashing
        print("Esperando reinicio del dispositivo...")
        time.sleep(5)
        
        # Open serial connection
        ser = serial.Serial(port, BAUD_RATE, timeout=CONFIG_TIMEOUT)
        
        # Give device time to initialize
        time.sleep(2)
        
        # Send configuration command
        print(f"Enviando configuración: {command}")
        command_bytes = (command + '\n').encode('utf-8')
        ser.write(command_bytes)
        
        # Wait for response
        time.sleep(3)
        
        # Read any response
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        ser.close()
        
        if response:
            print("Respuesta del dispositivo:")
            print(response)
        
        print("Configuración enviada exitosamente ✓")
        return True
        
    except serial.SerialException as e:
        print(f"ADVERTENCIA: Error enviando configuración: {e}")
        print("Puede configurar manualmente con:")
        print(f"  {command}")
        return False
    except Exception as e:
        print(f"ADVERTENCIA: Error inesperado: {e}")
        print("Configuración manual puede ser requerida")
        return False

def final_verification():
    """Perform final system verification"""
    print("[5/6] Verificación final...")
    print()
    
    # Check that all tools are still available
    tools_ok = True
    
    if not shutil.which('pio'):
        print("⚠ PlatformIO no encontrado en PATH")
        tools_ok = False
    else:
        print("✓ PlatformIO disponible")
    
    try:
        import serial
        print("✓ pyserial disponible")
    except ImportError:
        print("⚠ pyserial no disponible")
        tools_ok = False
    
    if tools_ok:
        print("✓ Todas las herramientas verificadas")
    else:
        print("⚠ Algunas herramientas pueden tener problemas")
    
    return tools_ok

def validate_args(args):
    """Validate command line arguments"""
    valid_roles = ['TRACKER', 'REPEATER', 'RECEIVER', 'END_NODE_REPEATER']
    valid_regions = ['US', 'EU', 'CH', 'AS', 'JP']
    valid_modes = ['SIMPLE', 'ADMIN']
    valid_profiles = ['DESERT_LONG_FAST', 'MOUNTAIN_STABLE', 'URBAN_DENSE', 
                     'MESH_MAX_NODES', 'CUSTOM_ADVANCED']
    
    errors = []
    
    if args.role not in valid_roles:
        errors.append(f"Rol inválido. Debe ser uno de: {', '.join(valid_roles)}")
    
    if not (1 <= args.id <= 999):
        errors.append("Device ID debe estar entre 1 y 999")
    
    if not (5 <= args.gps <= 3600):
        errors.append("Intervalo GPS debe estar entre 5 y 3600 segundos")
    
    if args.region not in valid_regions:
        errors.append(f"Región inválida. Debe ser una de: {', '.join(valid_regions)}")
    
    if args.mode not in valid_modes:
        errors.append(f"Modo de datos inválido. Debe ser uno de: {', '.join(valid_modes)}")
    
    if args.radio not in valid_profiles:
        errors.append(f"Perfil de radio inválido. Debe ser uno de: {', '.join(valid_profiles)}")
    
    if args.hops and not (1 <= args.hops <= 10):
        errors.append("Max hops debe estar entre 1 y 10")
    
    return errors

def print_success_summary(args, port):
    """Print successful configuration summary"""
    print()
    print("=" * 60)
    print("  ✅ PROCESO COMPLETADO")
    print("=" * 60)
    print()
    print("Configuración aplicada:")
    print(f"- Rol: {args.role}")
    print(f"- Device ID: {args.id}")
    print(f"- Intervalo GPS: {args.gps} segundos")
    print(f"- Región: {args.region}")
    print(f"- Modo de datos: {args.mode}")
    print(f"- Perfil de radio: {args.radio}")
    print(f"- Max Hops: {args.hops if args.hops else 3}")
    print()
    print("El dispositivo debería estar operando en modo normal.")
    print()
    print("Para monitorear el dispositivo:")
    print(f"  pio device monitor --port {port} --baud {BAUD_RATE}")
    print()
    print("Para futuras sesiones, PlatformIO ya está instalado.")
    print()

def show_usage():
    """Show usage information"""
    print()
    print(f"CUSTOM MESHTASTIC FLASH TOOL v{VERSION} - CON AUTO INSTALACIÓN")
    print()
    print("NUEVO: Este tool instala automáticamente PlatformIO si no está presente")
    print()
    print("USO:")
    print("  python3 flash_tool.py -role ROLE -id ID -gps INTERVAL -region REGION -mode MODE -radio PROFILE [-hops HOPS]")
    print()
    print("PARÁMETROS OBLIGATORIOS:")
    print("  -role ROLE           Rol del dispositivo: TRACKER, REPEATER, RECEIVER, END_NODE_REPEATER")
    print("  -id ID               Device ID único (1-999)")
    print("  -gps INTERVAL        Intervalo de lectura GPS en segundos (5-3600)")
    print("  -region REGION       Región LoRa: US, EU, CH, AS, JP")
    print("  -mode MODE           Modo de datos: SIMPLE, ADMIN")
    print("  -radio PROFILE       Perfil de radio: DESERT_LONG_FAST, MOUNTAIN_STABLE, URBAN_DENSE, MESH_MAX_NODES, CUSTOM_ADVANCED")
    print()
    print("PARÁMETROS OPCIONALES:")
    print("  -hops HOPS          Máximo saltos para mesh routing (1-10, default: 3)")
    print()
    print("PREREQUISITOS (se instalan automáticamente):")
    print("  ✓ Python 3.7+ (REQUERIDO - instalar manualmente)")
    print("  ✓ pip (se instala automáticamente si falta)")
    print("  ✓ PlatformIO Core (se instala automáticamente vía pip)")
    print("  ✓ pyserial (se instala automáticamente)")
    print()
    print("EJEMPLOS:")
    print("  # Tracker básico")
    print("  python3 flash_tool.py -role TRACKER -id 1 -gps 30 -region US -mode SIMPLE -radio DESERT_LONG_FAST")
    print()
    print("  # Repeater con configuración personalizada")
    print("  python3 flash_tool.py -role REPEATER -id 100 -gps 60 -region EU -mode ADMIN -radio MESH_MAX_NODES -hops 5")
    print()
    print("PERFILES DE RADIO DISPONIBLES:")
    print("  DESERT_LONG_FAST   - Máximo alcance (~8km, batería 3/10, velocidad 2/10)")
    print("                       Ideal para: Animal tracking, monitoreo de campo, sensores remotos")
    print()
    print("  MOUNTAIN_STABLE    - Condiciones adversas (~4km, batería 5/10, velocidad 4/10)")
    print("                       Ideal para: Repetidores en bosques, despliegues montañosos")
    print()
    print("  URBAN_DENSE        - Alta velocidad (~800m, batería 8/10, velocidad 9/10)")
    print("                       Ideal para: Testing en laboratorio, IoT urbano, desarrollo")
    print()
    print("  MESH_MAX_NODES     - Balance para redes grandes (~2.5km, batería 7/10, velocidad 7/10)")
    print("                       Ideal para: Redes mesh de 20-30 nodos, balance general")
    print()
    print("  CUSTOM_ADVANCED    - Configuración manual experta")
    print("                       Ideal para: Requisitos específicos, configuración avanzada")
    print()
    print("REGIONES LoRa:")
    print("  US  - Estados Unidos, México, Canadá (915 MHz)")
    print("  EU  - Europa, África, Medio Oriente (868 MHz)")
    print("  CH  - China (470 MHz)")
    print("  AS  - Asia general (433 MHz)")
    print("  JP  - Japón (920 MHz)")
    print()
    print("PRIMERA VEZ:")
    print("  1. Instale Python 3.7+ (python.org, brew, apt, etc.)")
    print("  2. Este script instalará todo lo demás automáticamente")
    print("  3. Conecte su ESP32-S3 y ejecute el comando")
    print()

def main():
    """Main function"""
    print_header()
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Custom Meshtastic Flash Tool', add_help=False)
    parser.add_argument('-role', required=True, help='Device role')
    parser.add_argument('-id', type=int, required=True, help='Device ID')
    parser.add_argument('-gps', type=int, required=True, help='GPS interval')
    parser.add_argument('-region', required=True, help='LoRa region')
    parser.add_argument('-mode', required=True, help='Data mode')
    parser.add_argument('-radio', required=True, help='Radio profile')
    parser.add_argument('-hops', type=int, help='Max hops (optional)')
    parser.add_argument('-h', '--help', action='store_true', help='Show help')
    
    # Handle help or no arguments
    if len(sys.argv) == 1 or '-h' in sys.argv or '--help' in sys.argv:
        show_usage()
        sys.exit(0)
    
    try:
        args = parser.parse_args()
    except SystemExit:
        show_usage()
        sys.exit(1)
    
    # Validate arguments
    errors = validate_args(args)
    if errors:
        print("ERRORES DE VALIDACIÓN:")
        for error in errors:
            print(f"  - {error}")
        print()
        show_usage()
        sys.exit(1)
    
    # Check if we're in a PlatformIO project directory
    if not os.path.exists('platformio.ini'):
        print("ERROR: No se encontró platformio.ini en el directorio actual")
        print("Por favor ejecute este script desde la raíz del proyecto PlatformIO")
        sys.exit(1)
    
    print("Configuración a aplicar:")
    print(f"- Rol: {args.role}")
    print(f"- Device ID: {args.id}")
    print(f"- Intervalo GPS: {args.gps}s")
    print(f"- Región: {args.region}")
    print(f"- Modo de datos: {args.mode}")
    print(f"- Perfil de radio: {args.radio}")
    print(f"- Max Hops: {args.hops if args.hops else 3}")
    print()
    
    print("=========================================================")
    print("  INICIANDO PROCESO DE SETUP Y FLASH")
    print("=========================================================")
    print()
    
    # Step 0: Check Python and install dependencies
    if not check_python_version():
        sys.exit(1)
    
    if not check_and_install_pip():
        sys.exit(1)
    
    if not install_python_dependencies():
        sys.exit(1)
    
    if not check_and_install_platformio():
        sys.exit(1)
    
    print()
    print("========== DEPENDENCIAS VERIFICADAS ==========")
    print("✓ Python disponible")
    print("✓ pip disponible")
    print("✓ pyserial disponible")
    print("✓ PlatformIO Core disponible")
    print()
    
    # Step 1: Detect ESP32 port
    port = detect_esp32_port()
    if not port:
        print("No se puede continuar sin un puerto válido")
        sys.exit(1)
    
    # Step 2: Install project dependencies
    install_project_dependencies()
    
    # Step 3: Flash firmware
    if not flash_firmware(port):
        sys.exit(1)
    
    # Step 4: Configure device
    # Build configuration command
    if args.hops:
        config_cmd = f"Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}"
    else:
        config_cmd = f"Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio}"
    
    send_config_command(port, config_cmd)
    
    # Step 5: Final verification
    final_verification()
    
    # Show success summary
    print_success_summary(args, port)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nOperación cancelada por usuario")
        sys.exit(1)
    except Exception as e:
        print(f"\nERROR INESPERADO: {e}")
        print("Por favor reporte este error con los detalles de su sistema")
        sys.exit(1)