Fase 2 — Criptografía de Canal y Claves
======================================

Objetivo
- Implementar cifrado de canal compatible con Meshtastic para `Data` dentro de `MeshPacket` usando PSK de 0/16/32 bytes.

Diseño
- Clave PSK según `channel.proto` (`psk` 0/16/32 bytes). 0 = sin cifrado.
- Algoritmo: AES-CTR (128 o 256 bits) sobre el submensaje `Data`.
- Nonce (16 bytes), mismo esquema que upstream (`CryptoEngine::initNonce`):
  - 64 bits: `packetId` en little-endian
  - 32 bits: `fromNode` en little-endian
  - 32 bits: contador de bloque (inicia en 0)

Implementación agregada
- `src/interop/meshtastic/crypto_channel.h/.cpp`:
  - `ChannelCrypto::set_psk(...)`, `encrypt(...)`, `decrypt(...)`, `make_nonce(...)`.
  - ESP32: `mbedtls_aes_crypt_ctr`.
  - nRF52: `rweather/Crypto` (AES128/256 + CTR) vía PlatformIO (`platformio.ini` ya lo incluye para `seeed_xiao_nrf52840`).
- `src/interop/meshtastic/packet_examples.h`:
  - `buildEncryptedFromDecoded(...)`: toma un `MeshPacketMsg` con `decoded(Data)`, cifra `Data` y devuelve bytes del `MeshPacket` con `encrypted`.

Uso típico
```c++
using namespace interop::meshtastic;

ChannelCrypto crypto;
uint8_t psk[16] = { /* 16 bytes */ };
crypto.set_psk(psk, sizeof(psk));

MeshPacketMsg mp; // llenar: from, id, channel, decoded (Data)
std::vector<uint8_t> wire;
bool ok = buildEncryptedFromDecoded(mp, crypto, wire);
```

Notas
- Si `psk` es de longitud 0, `encrypt/decrypt` actúan como no-op (compatibilidad con canales abiertos).
- El `packetId` usado en el nonce es el `MeshPacket.id` (en upstream puede ser 64-bit interno; aquí usamos 32-bit del mensaje para mantener compatibilidad práctica). Si se requiere 64-bit interno, se ajustará en Fase 3/4.
- En Fase 3 se integrará con la capa de radio y el encabezado OTA.

Soporte por plataforma
- ESP32: operativo (mbedTLS incluido en core Arduino-ESP32).
- nRF52 (XIAO nRF52840): operativo con librería `rweather/Crypto` (añadida en `platformio.ini`).
- Otras: se añadirá backend bajo demanda.

Próximo
- Fase 3: Inicialización radio (US915 + Urban_DENSE), pruebas de intercambio básico OTA.
- Fase 4: Interop mínima con un nodo Meshtastic (texto cifrado) y deduplicación/TTL.
