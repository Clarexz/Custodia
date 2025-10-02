#pragma once

#include "proto_min.h"
#include <string>

namespace interop {
namespace meshtastic {

// Utilidad para construir un MeshPacket con Data TEXT_MESSAGE_APP
inline std::vector<uint8_t> buildTextDecodedMeshPacket(
    uint32_t from,
    uint32_t to,
    uint32_t channelIndex,
    uint32_t packetId,
    uint32_t hopLimit,
    const std::string& text)
{
    MeshPacketMsg mp;
    mp.has_from = true; mp.from = from;
    mp.has_to = true; mp.to = to; // 0 = broadcast en muchos casos
    mp.has_channel = true; mp.channel = channelIndex; // 0 = primary
    mp.has_id = true; mp.id = packetId;
    mp.has_hop_limit = true; mp.hop_limit = hopLimit;
    mp.has_want_ack = true; mp.want_ack = false;

    mp.has_decoded = true;
    mp.decoded.portnum = 1; // TEXT_MESSAGE_APP
    mp.decoded.payload.assign(text.begin(), text.end());

    std::vector<uint8_t> out;
    encodeMeshPacket(mp, out);
    return out;
}

// Convierte un MeshPacket con decoded(Data) a uno con encrypted(bytes) usando AES-CTR de canal
inline bool buildEncryptedFromDecoded(const MeshPacketMsg& in,
                                      const ChannelCrypto& crypto,
                                      std::vector<uint8_t>& out_bytes)
{
    if (!in.has_decoded) return false;

    // Codificar Data claro
    std::vector<uint8_t> plain;
    encodeData(in.decoded, plain);

    // Cifrar (no-op si no hay clave)
    std::vector<uint8_t> cipher = plain;
    uint64_t pid = in.has_id ? static_cast<uint64_t>(in.id) : 0ULL;
    if (!crypto.encrypt(in.has_from ? in.from : 0, pid, cipher.data(), cipher.size()))
        return false;

    MeshPacketMsg mp = in;
    mp.has_encrypted = true;
    mp.encrypted = std::move(cipher);
    mp.has_decoded = false;

    out_bytes.clear();
    encodeMeshPacket(mp, out_bytes);
    return true;
}

} // namespace meshtastic
} // namespace interop
