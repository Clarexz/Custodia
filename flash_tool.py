#!/usr/bin/env python3
"""
Custom Meshtastic Flash Tool - SIMPLE VERSION
Only the essentials: flash + configure
Auto-installs pyserial if needed
"""

import sys
import subprocess
import time
import argparse

def check_and_install_pyserial():
    """Install pyserial if not available"""
    try:
        import serial
        import serial.tools.list_ports
        return True
    except ImportError:
        print("Instalando pyserial...")
        try:
            subprocess.run([sys.executable, '-m', 'pip', 'install', 'pyserial'], 
                          check=True, capture_output=True)
            print("pyserial instalado correctamente")
            return True
        except subprocess.CalledProcessError:
            print("ERROR: No se pudo instalar pyserial")
            print("Solucion: pip install pyserial")
            return False
        except FileNotFoundError:
            print("ERROR: Python no encontrado en PATH")
            print("Instale Python desde python.org e incluya en PATH")
            return False

def detect_port():
    """Simple port detection"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    
    # Look for common ESP32 patterns
    for port in ports:
        device_lower = port.device.lower()
        desc_lower = port.description.lower()
        
        if any(pattern in device_lower for pattern in ['usbmodem', 'usbserial', 'ttyusb', 'ttyacm']):
            return port.device
        if any(pattern in desc_lower for pattern in ['cp210', 'ch340', 'silicon labs']):
            return port.device
    
    # If no pattern match, use first available
    if ports:
        return ports[0].device
    
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

def main():
    print("Custom Meshtastic Flash Tool - Simple Version")
    
    # Check Python and install pyserial
    if not check_and_install_pyserial():
        sys.exit(1)
    
    # Now we can import serial after installation
    import serial
    
    # Parse arguments
    parser = argparse.ArgumentParser(description='Flash and configure ESP32-S3')
    parser.add_argument('-role', required=True, help='Device role')
    parser.add_argument('-id', type=int, required=True, help='Device ID')
    parser.add_argument('-gps', type=int, required=True, help='GPS interval')
    parser.add_argument('-region', required=True, help='LoRa region')
    parser.add_argument('-mode', required=True, help='Data mode')
    parser.add_argument('-radio', required=True, help='Radio profile')
    parser.add_argument('-hops', type=int, default=3, help='Max hops (default: 3)')
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)
    
    args = parser.parse_args()
    
    print(f"Configurando: {args.role} ID:{args.id} GPS:{args.gps}s {args.region} {args.mode} {args.radio}")
    
    # Find PlatformIO
    pio_cmd = find_platformio()
    if not pio_cmd:
        print("ERROR: PlatformIO no encontrado")
        print("Instale PlatformIO: pip install platformio")
        print("O agregue al PATH: export PATH=$PATH:~/.platformio/penv/bin")
        sys.exit(1)
    
    print(f"Usando PlatformIO: {pio_cmd}")
    
    # Step 1: Flash firmware (back to working version)
    print("\n[1/2] Flasheando firmware...")
    
    print("Detectando puerto para referencia...")
    port = detect_port()
    if port:
        print(f"Puerto detectado: {port}")
    else:
        print("ADVERTENCIA: No se detectó puerto específico")
    
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
    print("\n[2/2] Configurando dispositivo...")
    time.sleep(8)  # Wait for device reboot
    
    # Re-detect port after flash (may have changed)
    port = detect_port()
    if not port:
        print("ADVERTENCIA: No se detectó puerto después del flash")
        print("Configuración manual requerida")
        return
    
    print(f"Usando puerto para configuracion: {port}")
    
    # Build configuration command - try both variants
    config_cmd_q = f"Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}"
    config_cmd_alt = f"CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}"
    
    # Send configuration
    try:
        ser = serial.Serial(port, 115200, timeout=10)
        time.sleep(3)  # Wait for device initialization
        
        # Try Q_CONFIG first
        print(f"Enviando: {config_cmd_q}")
        ser.write((config_cmd_q + '\r\n').encode())
        time.sleep(3)  # Wait for response
        
        # Read response
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        # If Q_CONFIG failed, try alternative
        if 'comando desconocido' in response.lower() or 'unknown command' in response.lower():
            print("Q_CONFIG no reconocido, probando comando alternativo...")
            ser.write((config_cmd_alt + '\r\n').encode())
            time.sleep(3)
            
            # Read new response
            while ser.in_waiting > 0:
                response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)
        
        ser.close()
        
        # Check if configuration was successful
        if any(word in response.lower() for word in ['completada exitosamente', 'configuracion guardada', 'listo y operando']):
            print("Configuracion aplicada correctamente")
            print("Dispositivo listo para usar")
        else:
            print("ADVERTENCIA: Configuracion puede haber fallado")
            print("Verificacion manual:")
            print(f"1. pio device monitor --baud 115200 --port {port}")
            print("2. Ejecutar: STATUS")
            print(f"3. Si no configurado, enviar manualmente:")
            print(f"   {config_cmd_q}")
            print(f"   O alternativamente: {config_cmd_alt}")
            print("4. Luego: CONFIG_SAVE")
            print("5. Finalmente: START")
            
    except Exception as e:
        print(f"ERROR configurando: {e}")
        print("Configuracion manual:")
        print(f"1. pio device monitor --baud 115200")
        print(f"2. Enviar: Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}")
        print(f"3. Si Q_CONFIG falla, enviar: CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}")
        print("4. Luego: CONFIG_SAVE")
        print("5. Finalmente: START")

if __name__ == "__main__":
    main()