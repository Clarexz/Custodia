#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace interop {
namespace meshtastic {

struct ChannelKey {
    std::vector<uint8_t> bytes; // 0 (clear), 16 (AES128) o 32 (AES256)
    bool valid() const { return bytes.size() == 0 || bytes.size() == 16 || bytes.size() == 32; }
    bool enabled() const { return bytes.size() == 16 || bytes.size() == 32; }
};

// AES-CTR sobre nonce de 16 bytes = [packetId:64 LE][fromNode:32 LE][blockCounter:32 LE]
class ChannelCrypto {
  public:
    ChannelCrypto() = default;

    bool set_psk(const uint8_t* psk, size_t len);
    const ChannelKey& key() const { return key_; }

    // Cifra/descifra in-place. Si no hay clave, retorna true sin modificar.
    bool crypt_in_place(uint32_t fromNode, uint64_t packetId, uint8_t* bytes, size_t len) const;

    // Helpers explícitos
    bool encrypt(uint32_t fromNode, uint64_t packetId, uint8_t* bytes, size_t len) const { return crypt_in_place(fromNode, packetId, bytes, len); }
    bool decrypt(uint32_t fromNode, uint64_t packetId, uint8_t* bytes, size_t len) const { return crypt_in_place(fromNode, packetId, bytes, len); }

    static void make_nonce(uint32_t fromNode, uint64_t packetId, uint8_t nonce[16]);

  private:
    ChannelKey key_{};
};

} // namespace meshtastic
} // namespace interop

