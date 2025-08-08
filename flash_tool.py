#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import sys
import time
import subprocess
import importlib.util

def check_and_install_pyserial():
    """Check if pyserial is installed, install if not"""
    if importlib.util.find_spec("serial") is None:
        print("pyserial no está instalado. Instalando...")
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "pyserial"])
            print("pyserial instalado exitosamente")
            return True
        except subprocess.CalledProcessError:
            print("ERROR: No se pudo instalar pyserial")
            print("Instale manualmente: pip install pyserial")
            return False
    return True

def detect_port():
    """Detect ESP32-S3 port automatically"""
    try:
        import serial.tools.list_ports
        
        # Common ESP32-S3 identifiers
        esp32_identifiers = [
            'CP210',  # Silicon Labs CP210x
            'CH340',  # WCH CH340
            'FTDI',   # FTDI chips
            'USB',    # Generic USB serial
            '10C4:EA60',  # CP210x VID:PID
            '1A86:7523',  # CH340 VID:PID
        ]
        
        ports = serial.tools.list_ports.comports()
        for port in ports:
            port_info = f"{port.device} {port.description} {port.hwid}".upper()
            
            # Check for ESP32-S3 identifiers
            for identifier in esp32_identifiers:
                if identifier in port_info:
                    return port.device
        
        # If no specific match, return first available port
        if ports:
            return ports[0].device
            
    except ImportError:
        pass  # pyserial not available yet
    except Exception as e:
        print(f"Error detecting port: {e}")
    
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

def get_psk_from_device(ser, channel_name):
    try:
        print("Obteniendo PSK del dispositivo...")
        
        # Limpiar buffer
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        # Entrar a modo CONFIG
        ser.write(b'CONFIG\r\n')
        time.sleep(2)
        
        # Leer respuesta de CONFIG
        config_response = ""
        for _ in range(10):
            if ser.in_waiting > 0:
                config_response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.3)
        
        # Verificar entrada a CONFIG
        config_success = any(phrase in config_response for phrase in [
            'Entrando en modo configuración',
            'modo configuración', 
            'config',
            'CONFIG'
        ])
        
        if not config_success:
            print("No se pudo entrar a modo configuración")
            return None
        
        # Ejecutar NETWORK_SHOW_PSK
        ser.write(b'NETWORK_SHOW_PSK\r\n')
        time.sleep(3)
        
        # Leer respuesta del comando PSK
        psk_response = ""
        for _ in range(20):
            if ser.in_waiting > 0:
                chunk = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                psk_response += chunk
            time.sleep(0.4)
            
            # Buscar indicadores de respuesta completa
            if any(indicator in psk_response for indicator in ['PSK:', '====', 'bytes']):
                time.sleep(1)
                break
        
        # Extraer PSK
        extracted_psk = extract_psk_from_response(psk_response)
        
        # Regresar a modo operativo
        ser.write(b'START\r\n')
        time.sleep(2)
        
        # Leer respuesta de START (sin mostrar)
        for _ in range(8):
            if ser.in_waiting > 0:
                ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.3)
        
        return extracted_psk
        
    except Exception as e:
        print(f"Error obteniendo PSK: {e}")
        # Intentar regresar a START en caso de error
        try:
            ser.write(b'START\r\n')
            time.sleep(1)
        except:
            pass
        return None

def extract_psk_from_response(response):
    """
    Extraer PSK de la respuesta - VERSIÓN LIMPIA
    """
    try:
        lines = response.split('\n')
        
        # Buscar patrones de PSK
        psk_patterns = ['PSK:', '[INFO] PSK:', 'Comando:']
        
        for line in lines:
            line = line.strip()
            
            for pattern in psk_patterns:
                if pattern in line:
                    # Extraer PSK
                    psk_start = line.find(pattern) + len(pattern)
                    psk_part = line[psk_start:].strip()
                    
                    # Si contiene "--psk", extraer después de eso
                    if '--psk' in psk_part:
                        psk_start = psk_part.find('--psk') + 5
                        psk_part = psk_part[psk_start:].strip()
                    
                    # Limpiar PSK
                    psk = psk_part.replace('\r', '').replace('\n', '').strip()
                    
                    # Validar PSK (Base64)
                    if len(psk) > 15 and ('=' in psk or '+' in psk or '/' in psk):
                        return psk
        
        # Fallback: buscar cualquier string Base64
        import re
        base64_pattern = r'[A-Za-z0-9+/]{20,}={0,2}'
        
        for line in lines:
            matches = re.findall(base64_pattern, line)
            for match in matches:
                if len(match) > 20:
                    return match
        
        return None
        
    except Exception as e:
        return None

# TAMBIÉN LIMPIAR show_psk_copy_option():

def show_psk_copy_option(ser, channel_name):
    print("\n" + "="*60)
    print("DISPOSITIVO LISTO PARA USAR")
    print("="*60)
    
    while True:
        response = input("¿Ver PSK para usar en otro dispositivo? (Y/N): ").strip().upper()
        if response in ['Y', 'YES', 'S', 'SI']:
            # Obtener PSK del dispositivo
            psk = get_psk_from_device(ser, channel_name)
            if psk:
                print(f"\n--psk {psk}")
                print(f"\nCopia y agrega al final de tu próximo comando.")
            else:
                print("\nNo se pudo obtener PSK automáticamente")
                print("   Usa: config> NETWORK_SHOW_PSK para verlo manualmente")
            break
        elif response in ['N', 'NO']:
            print("PSK no mostrado. Puedes verlo después con: config> NETWORK_SHOW_PSK")
            break
        else:
            print("Por favor responde Y o N")

def main():
    print("Custodia Flash Tool v2.2")
    
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
    parser.add_argument('-channel', default='default', help='Network security channel name (default: default)')
    # NUEVO: Argumento --psk
    parser.add_argument('-psk', help='Pre-shared key (Base64). If provided, uses specific PSK instead of auto-generated')
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)
    
    args = parser.parse_args()
    
    print(f"Configurando: {args.role} ID:{args.id} GPS:{args.gps}s {args.region} {args.mode} {args.radio} {args.channel}")
    
    if args.channel and args.channel != 'default':
        print(f"Canal de seguridad: {args.channel}")
        print(f"   - Se creará una red mesh privada")
        print(f"   - Solo dispositivos con este canal pueden comunicarse")
    else:
        print("Usando configuración de red pública")
    
    # Find PlatformIO
    pio_cmd = find_platformio()
    if not pio_cmd:
        print("ERROR: PlatformIO no encontrado")
        print("Instale PlatformIO: pip install platformio")
        print("O agregue al PATH: export PATH=$PATH:~/.platformio/penv/bin")
        sys.exit(1)
    
    print(f"Usando PlatformIO: {pio_cmd}")
    
    # Step 1: Flash firmware (back to working version)
    print("\n[1/3] Flasheando firmware...")
    
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
    print("\n[2/3] Configurando dispositivo...")
    time.sleep(8)  # Wait for device reboot
    
    # Re-detect port after flash (may have changed)
    port = detect_port()
    if not port:
        print("ADVERTENCIA: No se detectó puerto después del flash")
        print("Configuración manual requerida")
        return
    
    print(f"Usando puerto para configuracion: {port}")
    
    print(f"[DEBUG] PSK proporcionado: '{args.psk}'")
    # Build configuration command - try both variants
    config_cmd_q = f"Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops},{args.channel},{args.psk}"
    print(f"[DEBUG] Comando Q_CONFIG: '{config_cmd_q}'")
    
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
            
            if "modo operativo" in start_response.lower() or "running" in start_response.lower():
                print("Dispositivo configurado y iniciado correctamente")
                
                # Show security configuration
                print(f"\nCONFIGURACIÓN DE SEGURIDAD:")
                print(f"   Canal configurado: {args.channel}")
                print(f"   El dispositivo está en una red mesh privada")
                print(f"   Solo dispositivos con el mismo canal pueden comunicarse")
                print(f"\nPara verificar: pio device monitor --baud 115200")
                print(f"   Luego ejecutar: NETWORK_LIST")
            else:
                print("Configuración parcial - verificar manualmente")
        
        elif config_success:
            print("Configuración básica completada")
            if args.channel == 'default':
                print("Usando red mesh pública (sin encriptación)")
        
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
            if args.channel and args.channel != 'default':
                print(f"4. Configurar canal de seguridad:")
                print(f"   NETWORK_CREATE {args.channel}")
                print(f"5. Verificar: NETWORK_LIST")
                print(f"6. Finalmente: START")
            else:
                print("4. Finalmente: START")
            print("4. Luego: CONFIG_SAVE")
            print("5. Finalmente: START")
            
    except Exception as e:
        print(f"ERROR configurando: {e}")
        print("Configuracion manual:")
        print(f"1. pio device monitor --baud 115200")
        print(f"2. Enviar: Q_CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}")
        print(f"3. Si Q_CONFIG falla, enviar: CONFIG {args.role},{args.id},{args.gps},{args.region},{args.mode},{args.radio},{args.hops}")
        if args.channel and args.channel != 'default':
            print(f"4. Configurar canal de seguridad:")
            print(f"   NETWORK_CREATE {args.channel}")
            print(f"5. Verificar configuración:")
            print(f"   NETWORK_LIST")
            print(f"6. Finalmente: START")
        else:
            print("4. Finalmente: START")
        print("4. Luego: CONFIG_SAVE")
        print("5. Finalmente: START")

if __name__ == "__main__":
    main()