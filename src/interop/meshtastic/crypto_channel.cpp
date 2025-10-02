#include "crypto_channel.h"
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include "mbedtls/aes.h"
#elif defined(ARDUINO_ARCH_NRF52)
#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#endif

namespace interop {
namespace meshtastic {

bool ChannelCrypto::set_psk(const uint8_t* psk, size_t len)
{
    key_.bytes.assign(psk, psk + len);
    return key_.valid();
}

void ChannelCrypto::make_nonce(uint32_t fromNode, uint64_t packetId, uint8_t nonce[16])
{
    std::memset(nonce, 0, 16);
    // 64-bit packetId LE
    std::memcpy(nonce + 0, &packetId, sizeof(uint64_t));
    // 32-bit fromNode LE
    std::memcpy(nonce + 8, &fromNode, sizeof(uint32_t));
    // last 4 bytes: block counter, inicia en 0 (mbedtls ctr se encargará)
}

bool ChannelCrypto::crypt_in_place(uint32_t fromNode, uint64_t packetId, uint8_t* bytes, size_t len) const
{
    if (!key_.enabled()) {
        // sin cifrado
        return key_.valid();
    }

#if defined(ARDUINO_ARCH_ESP32)
    uint8_t nonce[16];
    make_nonce(fromNode, packetId, nonce);

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int ret = 0;
    const int keybits = (key_.bytes.size() == 32) ? 256 : 128;
    ret = mbedtls_aes_setkey_enc(&ctx, key_.bytes.data(), keybits);
    if (ret != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }

    size_t nc_off = 0;
    uint8_t nonce_counter[16];
    uint8_t stream_block[16];
    std::memcpy(nonce_counter, nonce, 16);
    std::memset(stream_block, 0, sizeof(stream_block));

    // mbedtls soporta in-place
    ret = mbedtls_aes_crypt_ctr(&ctx, len, &nc_off, nonce_counter, stream_block, bytes, bytes);
    mbedtls_aes_free(&ctx);
    return ret == 0;
#elif defined(ARDUINO_ARCH_NRF52)
    uint8_t nonce[16];
    make_nonce(fromNode, packetId, nonce);

    // Selección de tamaño de clave
    if (key_.bytes.size() == 16) {
        CTR<AES128> ctr;
        ctr.setKey(key_.bytes.data(), key_.bytes.size());
        ctr.setIV(nonce, 16);
        ctr.setCounterSize(4);
        std::vector<uint8_t> scratch(bytes, bytes + len);
        ctr.encrypt(bytes, scratch.data(), len);
        return true;
    } else if (key_.bytes.size() == 32) {
        CTR<AES256> ctr;
        ctr.setKey(key_.bytes.data(), key_.bytes.size());
        ctr.setIV(nonce, 16);
        ctr.setCounterSize(4);
        std::vector<uint8_t> scratch(bytes, bytes + len);
        ctr.encrypt(bytes, scratch.data(), len);
        return true;
    }
    return false;
#else
    (void)fromNode; (void)packetId; (void)bytes; (void)len;
    // Backend no implementado para esta plataforma aún
    return false;
#endif
}

} // namespace meshtastic
} // namespace interop
