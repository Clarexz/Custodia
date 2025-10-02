# Resumen Fases 0–3 — Interoperabilidad Custodia × Meshtastic (enfoque QA)

Documento para QA del proyecto. Resume lo alcanzado (Fases 0–3), explica por qué se implementó así y justifica los artefactos nuevos (archivos/carpetas) incorporados al código. Copiar/pegar en Notion.

---

## En una mirada

- Objetivo: compatibilizar Custodia con Meshtastic (red mesh LoRa) empezando por mensajes, cifrado y radio.
- Estado: Fases 0, 1 y 2 completadas; Fase 3 implementada y lista para validación con preset LongFast.

---

## ¿Qué estamos construyendo?

- Meshtastic: ecosistema de radios que forman una red “mesh” (reenvío entre nodos) para mensajería/datos sin infraestructura.
- LoRa: tecnología de radio de largo alcance y bajo consumo usada por Meshtastic y Custodia.
- Interoperabilidad: que un Custodia pueda integrarse en esa red (enviar/recibir con el mismo formato y parámetros).

---

## ¿Por qué hacerlo por fases?

- Reducir riesgo: validaciones incrementales por capa (mensajes → cifrado → radio) acotan el origen de fallos.
- Aprender rápido: cada fase entrega métricas/fixtures que sirven de referencia para las siguientes.
- Reutilizar: lo ya probado se integra sin reescrituras cuando sumemos funcionalidades OTA.

---

## ¿Qué logramos en cada fase?

### Fase 0 — Alineación de protocolo y entorno
- Integramos un subset de Protocol Buffers (proto) de Meshtastic: definiciones de estructura/serialización.
- Preparamos build en ESP32S3 y nRF52840.

### Fase 1 — Librería de paquetes (sin radio)
- Construcción y parseo en memoria (IDs, hop limit, tipo de puerto). Ejemplos de mensajes de texto.

### Fase 2 — Criptografía y canales
- Cifrado por canal (AES-CTR) con nonce derivado de origen+ID. API `encrypt/decrypt` lista.

### Fase 3 — Driver radio y presets
- Radio SX1262 operativa en ambos targets. Perfil LongFast aplicado desde configuración (SF/BW/CR/Power/Preamble).
- Frecuencia por región (US/EU/AS/CH/JP). Comando `PING` para enlace básico y métricas (RSSI/SNR/airtime).

Resultado: dos Custodia se comunican entre sí con parámetros alineados a Meshtastic.

---

## ¿Cómo comprobamos que funciona?

- Ponemos dos equipos cerca (2–5 metros), con la misma región y perfil LongFast.
- Desde uno, enviamos un `PING` y el otro lo recibe. Miramos:
  - Calidad de señal (RSSI/SNR).
  - Tiempo en el aire (airtime) por transmisión.

Si ambos equipos se ven entre sí de forma confiable y los tiempos están dentro de lo esperado, la fase está aprobada.

---

## Beneficios de esta estrategia

- Claridad: cada etapa cierra un objetivo tangible (mensajes listos, cifrado listo, radio operativa).
- Menos sorpresas: al cruzar con un nodo Meshtastic oficial, lo básico (mensajes/cifrado/radio) ya está validado.
- Portabilidad: validamos en dos hardwares distintos desde temprano, evitando ajustes costosos después.

---

## Próximos pasos (a partir de la Fase 4)

- Probar intercambio de mensajes “reales” con un dispositivo Meshtastic oficial, ya con cifrado y formato finales.
- Luego sumar funciones visibles para usuario: información del dispositivo, posición, telemetría básica y reenvío en la malla.
- Más adelante, control por cable/Bluetooth desde la app de Meshtastic y mejoras de robustez (reintentos, control de ritmo, etc.).

---

## Glosario rápido

- Meshtastic: red de radios en malla para enviar mensajes sin internet.
- LoRa: tecnología de radio de largo alcance y bajo consumo.
- Protocol Buffers (proto): plantillas/contratos para serializar mensajes de forma compatible.
- Perfil de radio: conjunto de parámetros que define cómo se transmite (alcance, rapidez). LongFast es el perfil que equilibra ambas cosas y es el que usa Meshtastic por defecto.
- Región/frecuencia: cada país/región obliga a usar ciertas frecuencias; por eso configuramos US/EU/etc.
- PING: mensaje simple para comprobar que dos equipos se ven y medir la calidad del enlace.
- Calidad de señal (RSSI/SNR): números que indican qué tan fuerte y limpia llega la radio; los usamos para validar.
- Tiempo en el aire: duración de un envío; ayuda a estimar rapidez y uso del canal.

---

---

## Artefactos nuevos y justificación (QA)

- `src/interop/meshtastic/proto_min.h`, `src/interop/meshtastic/proto_min.cpp`
  - Subset mínimo de `MeshPacket` (id, hop_limit, portnum, payloads). Evita traer todos los proto upstream.
  - QA: facilita fixtures deterministas para encode/decode.

- `src/interop/meshtastic/crypto_channel.h`, `src/interop/meshtastic/crypto_channel.cpp`
  - Canal de cifrado AES-CTR con nonce a partir de origen+ID.
  - QA: permite vectores “golden” claro↔cifrado antes de OTA.

- `src/interop/meshtastic/packet_examples.h`
  - Helpers para construir mensajes de texto (decodificados/cifrados).
  - QA: acelera pruebas manuales e integración en Fase 4.

- `src/radio/radio_profiles.h`, `src/radio/radio_profiles.cpp`
  - Perfiles de radio (incl. LongFast) + gestor y cálculo de airtime.
  - QA: un único punto auditable de SF/BW/CR/Power/Preamble.

- `src/lora/lora_hardware.cpp`
  - Aplica perfil activo en `begin()`/`configureRadio()`; nuevos setters de CR/Preamble.
  - QA: los parámetros efectivos quedan trazados y consistentes entre targets.

- `src/lora/lora_manager.h/.cpp`
  - Métodos `setCodingRate()` y `setPreambleLength()`; `printConfiguration()` muestra perfil activo.
  - QA: inspección de configuración efectiva sin herramientas externas.

- `src/lora/lora_comm.cpp`
  - Permite payload cero (para `PING`); filtrado por `networkHash`.
  - QA: prueba de humo simple; menos crosstalk en banco.

- `src/serial/serial_handler.cpp`
  - Comandos `PING` y `PING <ID>`; ayuda en `HELP`.
  - QA: validación inmediata de RX/TX y airtime.

- `src/config/config_manager.cpp`
  - Default `radioProfile` = `LONG_FAST`; `CONFIG_REGION`/`CONFIG_RADIO_PROFILE` para control por serial.
  - QA: reduce fricción y homogeniza entorno de prueba.

- `docs/resumen_interop_fases_0_3.md`, `plan_interoperabilidad_meshtastic.md`
  - Documentan alcance, criterios y matriz de validación.

---

Última actualización: Fases 0–3 completas. Siguiente: pruebas cifradas con un nodo Meshtastic oficial (Fase 4).
