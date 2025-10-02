Fase 1 — Librería de Paquetes (sin radio)
=========================================

Objetivo
- Construir y parsear estructuras compatibles con Meshtastic para `MeshPacket` (subset) y `Data` sin involucrar radio ni cifrado.

Qué se agregó
- `src/interop/meshtastic/proto_min.h/.cpp`: encoder/decoder Protobuf mínimo (varint, length-delimited, fixed32) y modelos:
  - `DataMsg` (portnum, payload, dest/source opcional, request/reply opcional)
  - `MeshPacketMsg` (from/to/channel/id/hop_limit/want_ack + oneof decoded/encrypted)
- `src/interop/meshtastic/packet_examples.h`: helper `buildTextDecodedMeshPacket(...)` para crear un `MeshPacket` con `Data` TEXT_MESSAGE_APP.

Uso básico (ejemplo)
```c++
#include "interop/meshtastic/packet_examples.h"

void example() {
  // Construir un MeshPacket con Data (TEXT_MESSAGE_APP)
  auto bytes = interop::meshtastic::buildTextDecodedMeshPacket(
      /*from*/ 0x12345678,
      /*to*/   0,              // 0 = broadcast
      /*channelIndex*/ 0,      // 0 = primary
      /*packetId*/ 42,
      /*hopLimit*/ 3,
      /*text*/ "Hola mesh");

  // Decodificar nuevamente
  interop::meshtastic::MeshPacketMsg mp;
  bool ok = interop::meshtastic::decodeMeshPacket(bytes.data(), bytes.size(), mp);
}
```

Notas
- El encode/decode implementado es un subconjunto suficiente para `decoded` con `Data` y campos básicos de `MeshPacket`.
- En la Fase 2 se añadirá cifrado de canal y manejo de `encrypted`.
- La integración con la capa radio (encabezado LoRa de 16 bytes y timings) se hace en Fase 3.

