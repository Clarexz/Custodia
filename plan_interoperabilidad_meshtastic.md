# Plan de Interoperabilidad con Meshtastic

Este documento describe los retos técnicos y un plan de implementación por fases para lograr que el firmware de Custodia sea interoperable con el firmware oficial de Meshtastic. El plan sigue los lineamientos de trabajo por fases y bloques pequeños definidos en `CODEX_INSTRUCTIONS.md` y contempla validaciones en cada etapa para evitar regresiones.

---

## 1) Objetivo y Alcance

- Interoperabilidad OTA a nivel radio (LoRa): enviar/recibir paquetes compatibles con nodos Meshtastic y participar del enrutamiento de la red.
- Interoperabilidad de control/servicios: compatibilidad con el API `toRadio`/`fromRadio` (USB-serial/BLE) para uso con apps Meshtastic y herramientas CLI.
- Compatibilidad de puertos de aplicación esenciales: `TEXT_MESSAGE_APP`, `POSITION_APP`, `NODEINFO_APP`, `ROUTING_APP`, `TELEMETRY_APP` (mínimo lectura/propagación).
- Seguridad y canales: manejo de canales, claves y cifrado compatibles con versiones estables de Meshtastic.
- Validación cruzada contra al menos un dispositivo de referencia con firmware oficial.

Fuera de alcance inicial (fase posterior u opcional): puente MQTT, OTA/DFU propio, módulos avanzados (Range Test completo, Serial Module, Store&Forward con políticas específicas).

---

## 2) Retos Técnicos (visión completa)

### 2.1 Capa Radio (LoRa) y Regulaciones
- Alineación exacta de parámetros de modem: frecuencia, `Spreading Factor`, `Bandwidth`, `Coding Rate`, potencia TX, `Preamble Length` y sincronización de canal. Ver `meshtastic_radio_profiles.md`.
- Perfiles/modem presets idénticos a Meshtastic (e.g., LONG_FAST, LONG_SLOW, etc.). Todos los nodos de un canal deben compartir el perfil.
- Planes de frecuencia/regulación regional (EU 868 MHz, US 915 MHz, etc.), duty-cycle, potencia legal y hop-limit de red.
- Diferencias de hardware (SX127x vs SX126x): inicialización, tiempos de conmutación RX/TX, IRQs y sensibilidad.

### 2.2 Estructura de Paquetes y Protobuf
- Uso de los `.proto` oficiales (MeshPacket, DataPacket, Config, ModuleConfig, PortNums…). Versionado de mensajes y campos opcionales.
- Encapsulado sobre LoRa: encabezados, flags, `hop_limit/TTL`, `channel`/`modem_preset`, `id` de paquete, timestamps.
- Fragmentación y reensamblado para cargas grandes (multi-fragment con orden y detección de faltantes).

### 2.3 Seguridad y Canales
- Derivación/gestión de claves a partir de `channel URL` o configuración equivalente. Soporte para canales primarios/secundarios.
- Cifrado/AEAD según versión upstream (evitar suposiciones: implementar lo definido en su código y documentación; contemplar modos legacy y actuales si aplica).
- Rotación de claves, canales públicos/privados y control de acceso.

### 2.4 Enrutamiento, Fiabilidad y Control-Plane
- Flood routing con desduplicación por `packet_id` y ventanas de tiempo; `hop_limit` decreciente.
- ACKs, reintentos y ventanas de backoff para minimizar colisiones; políticas de reTX por tipo de puerto.
- Mantenimiento de base de nodos (NodeDB): `node_id`, `user`, `device metrics`, últimos vistos, roles/router.
- Rate limiting, airtime budgeting y fairness para redes densas.

### 2.5 Puertos de Aplicación (Portnums)
- Soporte mínimo interoperable: `TEXT_MESSAGE_APP`, `POSITION_APP`, `NODEINFO_APP`, `ROUTING_APP`, `TELEMETRY_APP`.
- Compatibilidad de semántica y campos: timestamps, prioridad, confirmaciones, formatos de contenido.

### 2.6 Interfaces de Control (BLE/USB-Serial)
- GATT Service y características compatibles (UUIDs Meshtastic) para `toRadio/fromRadio` y notificaciones.
- Protocolo en serie idéntico: framing, `toRadio`/`fromRadio`, mensajes de configuración/admin, respuesta a comandos.
- Descubrimiento y sincronización con apps oficiales (Android/iOS/CLI).

### 2.7 Gestión de Configuración
- `Config` y `ModuleConfig` protobuf: lectura/escritura, persistencia en NVS/flash y validación de límites por región.
- Migraciones de versión y defaults seguros; negociación de capacidades si hay campos no soportados.

### 2.8 Sincronización y Tiempos
- Timestamps coherentes, latencia, tiempo en aire y manejo de reloj (RTC/monotónico). Efectos en expiración/deduplicación.
- Tiempos de sleep/wake y latencia de recepción para ahorro energético.

### 2.9 Recursos y Rendimiento en MCU
- RAM/flash limitados: buffers de fragmentación, colas, NodeDB, pila protobuf y cifrado.
- ISR/RTOS: prioridad de interrupciones LoRa vs tareas de cifrado/serial/BLE.

### 2.10 Compatibilidad por Versión y Capacidades
- Alineación con una versión objetivo de Meshtastic; manejo de cambios futuros (feature flags/capabilities).
- Estrategia de “gracia” ante campos desconocidos en protobuf y downgrade seguro.

### 2.11 Pruebas e Interop
- Vectores “golden” de paquetes (cifrados/descifrados) para tests deterministas.
- Matriz de compatibilidad HW (radios y regiones) y funcional (puertos, BLE/serial, perfiles radio).
- Herramientas de sniffing/logging y pruebas OTA con dispositivos oficiales.

---

## 3) Suposiciones y Decisiones Iniciales

- Se adoptarán los `.proto` oficiales como fuente de verdad. No se forkearán salvo necesidad.
- Se fijará una versión objetivo de Meshtastic estable para la primera interoperabilidad y se documentará.
- Se soportarán, como mínimo, los perfiles `LONG_FAST` y `LONG_SLOW` del archivo `meshtastic_radio_profiles.md` para las pruebas iniciales.
- USB-serial será el primer canal de control a validar; BLE se activará después de comprobar estabilidad OTA.

---

## 4) Plan por Fases (alineado a CODEX_INSTRUCTIONS)

Cada fase entrega un bloque pequeño, con criterios de aceptación y validación cruzada. No se toca todo a la vez; se itera módulo por módulo.

### Fase 0 — Alineación de Protocolo y Entorno
- Entregables
  - Submódulo o copia controlada de `.proto` oficiales (MeshPacket, DataPacket, Config, ModuleConfig, Portnums).
  - Utilidad para compilar protobuf a C/C++ (o binding elegido) y tareas de build.
  - Documento interno con versión objetivo de Meshtastic y mapa de campos críticos.
- Validación
  - Compilación limpia del firmware con las definiciones `.proto` integradas.
  - Pruebas unitarias de serialización/deserialización de mensajes simples (no cifrado).

### Fase 1 — Librería de Paquetes (sin radio)
- Entregables
  - Construcción/parseo de `MeshPacket`/`DataPacket` compatibles (sin cifrar).
  - Manejo de `hop_limit`, `packet_id`, `portnum`, fragmentación/reensamblado en memoria.
  - Golden tests de encode/decode con fixtures reproducibles.
- Validación
  - 100% de pruebas unitarias de encode/decode y fragmentación pasan en CI local.

### Fase 2 — Criptografía y Canales
- Entregables
  - Implementación del esquema de claves y cifrado AEAD según versión upstream (incluyendo derivación desde `channel URL`).
  - Soporte para múltiples canales (primario/secundarios) y selección por `channel_index`.
  - Golden vectors: payloads cifrados/descifrados y verificación de autenticidad.
- Validación
  - Tests unitarios con casos válidos y de fallo (clave errónea, nonce repetido, MAC inválido).

### Fase 3 — Driver Radio y Modem Presets
- Entregables
  - Inicialización del transceptor (SX127x/SX126x) y perfil `LONG_FAST` estable.
  - Ajustes de región (frecuencia, potencia y duty-cycle) y canal estático.
  - RX/TX básico de tramas “vacías” y medición de RSSI/SNR.
- Validación
  - Dos nodos Custodia intercambian paquetes “ping” sin cifrar con `hop_limit` correcto.
  - Medición de airtime coherente con perfil configurado.

### Fase 4 — OTA interoperable mínima (canal cifrado + texto)
- Entregables
  - TX/RX de `MeshPacket` cifrados en `LONG_FAST` con un nodo Meshtastic oficial, en el mismo canal.
  - Soporte de `TEXT_MESSAGE_APP` punto a punto y broadcast; desduplicación y TTL.
- Validación
  - Mensajes de texto intercambiados bidireccionalmente con un dispositivo Meshtastic oficial.
  - Logs muestran `packet_id` únicos, dedupe y `hop_limit` decreciente correcto.

### Fase 5 — Puertos esenciales y NodeDB
- Entregables
  - `NODEINFO_APP`, `POSITION_APP`, `ROUTING_APP`, `TELEMETRY_APP` (mínimo reenvío/lectura; emisión básica cuando aplique).
  - Base de nodos con últimos vistos, user info y métricas mínimas.
- Validación
  - Custodia aparece con nombre/ID en apps Meshtastic; posición/telemetría se propaga.
  - Paquetes ROUTING se procesan sin romper el flujo de texto/posición.

### Fase 6 — Control por USB-Serial (toRadio/fromRadio)
- Entregables
  - Framing y procesamiento completo de `toRadio`/`fromRadio` vía serial.
  - Lectura/escritura de `Config`/`ModuleConfig`, reinicio controlado y persistencia.
- Validación
  - Meshtastic CLI/app de escritorio puede leer estado del nodo Custodia y cambiar parámetros básicos.

### Fase 7 — BLE GATT básico
- Entregables
  - Servicio y características Meshtastic; notificaciones para `fromRadio`.
  - Seguridad BLE coherente con la plataforma y pairing opcional.
- Validación
  - App móvil Meshtastic descubre el nodo Custodia, sincroniza NodeDB y envía/recibe mensajes.

### Fase 8 — Robustez: ACKs, Reintentos, Rate Limiting
- Entregables
  - Políticas de reintento por puerto, ventanas de backoff, rate limiting por airtime.
  - Persistencia breve de colas para tolerar resets leves.
- Validación
  - Pruebas de congestión con 3–5 nodos mixtos (Custodia + oficiales) sin pérdidas excesivas.

### Fase 9 — Compatibilidad extendida y Matriz Interop
- Entregables
  - Perfiles adicionales `LONG_SLOW` y `MEDIUM_*` según `meshtastic_radio_profiles.md`.
  - Pruebas cruzadas BLE/serial, múltiples canales, regiones, y varios portnums.
  - Documento de Matriz Interop: features vs HW/región.
- Validación
  - Checklist de interop al 100% para el conjunto de features comprometidas.

### Fase 10 — Documentación y Release Candidate
- Entregables
  - Guía de configuración y solución de problemas; notas de versión y límites conocidos.
  - Scripts de pruebas reproducibles y fixtures.
- Validación
  - Prueba de humo completa en hardware de referencia antes del tag RC.

---

## 5) Criterios de Aceptación por Fase (resumen)

- Build limpio, pruebas unitarias/funcionales pasan.
- Validación OTA con al menos un nodo Meshtastic oficial para fases 4+.
- No regresiones en funciones ya aceptadas en fases previas.
- Logs trazables: nivel DEBUG/INFO con IDs de paquetes, hop y resultados de cifrado.

---

## 6) Estrategia de Pruebas y Herramientas

- Unit tests: encode/decode protobuf, cifrado/AEAD, fragmentación, dedupe, TTL.
- Hardware-in-the-loop: pruebas OTA con bancos de nodos (Custodia + oficiales).
- Golden vectors: fixtures de paquetes (claro/cifrado) versionados en repo.
- Métricas: tasa de entrega, latencia, airtime estimado vs medido, consumo.
- Sniffing/logging: logs seriales y dumps binarios de paquetes para auditoría.

---

## 7) Riesgos y Mitigaciones

- Cambios de protocolo upstream: fijar versión objetivo, seguimiento y pruebas de compatibilidad.
- Limitaciones de RAM/flash: optimizar buffers, seleccionar subconjuntos de features por build.
- Diferencias de radio: abstraer driver y medir tiempos por chip; calibración.
- Cifrado/seguridad: usar libs robustas; tests negativos; cuidado con nonces/IVs.
- BLE inestable: priorizar USB-serial para control; BLE detrás de flag hasta estabilizar.

---

## 8) Dependencias y Requisitos

- Protobuf compiler/bindings para el toolchain del firmware.
- Librería crypto compatible con AEAD requerido por Meshtastic (y/o variante legacy si aplica).
- Drivers de radio (SX127x/SX126x) y capa de abstracción.
- Dispositivo Meshtastic oficial para pruebas comparativas.

---

## 9) Checklists de Validación Rápida

- Canal correcto: frecuencia, preset y claves coinciden con el nodo oficial.
- `hop_limit` decrece, no se reinyectan paquetes duplicados.
- Mensajes de texto se entregan en ambas direcciones; ACKs correctos.
- Aparición en app Meshtastic: NodeInfo visible; Position y Telemetry llegan.
- `toRadio/fromRadio` operativos por USB-serial; BLE opcional cuando habilitado.

---

## 10) Notas de Ejecución (CODEX_INSTRUCTIONS)

- Implementar en bloques pequeños por archivo/módulo.
- Proponer plan detallado al iniciar cada fase y solicitar confirmación antes de tocar código.
- Priorizar entregables funcionales y validables en hardware.
- Mantener documentación viva y fixtures de prueba actualizados en cada fase.

---

## 11) Próximos Pasos Propuestos

1. Confirmar versión objetivo de Meshtastic y hardware de referencia.
2. Ejecutar Fase 0: integrar `.proto` oficiales y compilar bindings.
3. Preparar banco de pruebas con al menos un nodo oficial y dos Custodia.

---

## 12) Referencias Técnicas y Ejemplos

- Repos oficiales
  - Firmware Meshtastic (código fuente): https://github.com/meshtastic/firmware
  - Instrucciones de build: https://meshtastic.org/docs/development/firmware/build
  - Protobufs oficiales (fuente de verdad del protocolo): https://github.com/meshtastic/protobufs
  - Directorio de mensajes `.proto`: https://github.com/meshtastic/protobufs/tree/master/meshtastic

- Mensajería y control (protobufs clave)
  - `mesh.proto` (incluye `FromRadio`/`ToRadio`, `MeshPacket`): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/mesh.proto
  - `portnums.proto` (puertos de aplicación): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/portnums.proto
  - `config.proto` (configuración): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/config.proto
  - `module_config.proto` (módulos): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/module_config.proto
  - `channel.proto` (canales): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/channel.proto
  - `telemetry.proto` (telemetría): https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/telemetry.proto

- Anclas a líneas relevantes (GitHub)
  - `mesh.proto`
    - Definición `FromRadio`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1882
    - Campo `FromRadio.payload_variant` (incluye `MeshPacket`): https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1892
    - Definición `ToRadio`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L2055
    - Campo `ToRadio.packet`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L2063
    - Campo `ToRadio.want_config_id`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L2075
    - Campo `ToRadio.heartbeat`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L2098
    - Definición `MeshPacket`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1198
    - `MeshPacket.from`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1345
    - `MeshPacket.to`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1350
    - `MeshPacket.channel` (índice/hash local): https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1361
    - `MeshPacket.payload_variant.decoded`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1376
    - `MeshPacket.payload_variant.encrypted`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1381
    - `MeshPacket.id`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1394
    - `MeshPacket.rx_time`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1402
    - `MeshPacket.hop_limit`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1417
    - `MeshPacket.want_ack`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1430
    - `MeshPacket.priority`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1436
  - `portnums.proto` (puertos clave)
    - `TEXT_MESSAGE_APP = 1;`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto#L40
    - `POSITION_APP = 3;`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto#L54
    - `NODEINFO_APP = 4;`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto#L61
    - `ROUTING_APP = 5;`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto#L68
    - `TELEMETRY_APP = 67;`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/portnums.proto#L166
  - `clientonly.proto`
    - `DeviceProfile.channel_url`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/clientonly.proto#L32
  - `channel.proto`
    - `ChannelSettings.psk` (tamaños válidos 0/16/32 bytes): https://github.com/meshtastic/protobufs/blob/master/meshtastic/channel.proto#L46
  - `mesh.proto` (estructura `Data` dentro de `MeshPacket`)
    - `Data.portnum`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1043
    - `Data.payload`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1048
    - `Data.dest` y `Data.source`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto#L1064
  - `config.proto` (roles y comportamiento)
    - `Config.DeviceConfig.Role` (inicio de enum): https://github.com/meshtastic/protobufs/blob/master/meshtastic/config.proto#L21
    - `Config.DeviceConfig.RebroadcastMode`: https://github.com/meshtastic/protobufs/blob/master/meshtastic/config.proto#L125
  - Cifrado en firmware (`CryptoEngine`)
    - Estructura del nonce (comentario): https://github.com/meshtastic/firmware/blob/master/src/mesh/CryptoEngine.h#L89
    - `encryptPacket` (AES-CTR sobre canal): https://github.com/meshtastic/firmware/blob/master/src/mesh/CryptoEngine.cpp#L218
    - `initNonce` (composición packetId/fromNode/extra): https://github.com/meshtastic/firmware/blob/master/src/mesh/CryptoEngine.cpp#L259
    - `encryptCurve25519` (AEAD CCM con ECDH+SHA256): https://github.com/meshtastic/firmware/blob/master/src/mesh/CryptoEngine.cpp#L82
    - `decryptCurve25519`: https://github.com/meshtastic/firmware/blob/master/src/mesh/CryptoEngine.cpp#L122

- Capa de cifrado (referencia de implementación upstream)
  - `CryptoEngine.h`: https://raw.githubusercontent.com/meshtastic/firmware/master/src/mesh/CryptoEngine.h
  - `CryptoEngine.cpp`: https://raw.githubusercontent.com/meshtastic/firmware/master/src/mesh/CryptoEngine.cpp

- Radio y canales
  - Configuración de canales (documentación): https://meshtastic.org/docs/configuration/radio/channels/
  - Perfiles de radio (resumen en este repo): `meshtastic_radio_profiles.md`

- Ejemplos/terceros relacionados
  - MiniMeshT (cliente terminal interoperable por BLE/Serial/TCP): https://github.com/allanrbo/MiniMeshT
  - flipper-meshtastic (port/experimento Flipper Zero + SX1262): https://github.com/jeofo/flipper-meshtastic
