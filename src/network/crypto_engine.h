/*
 * CRYPTO_ENGINE.H - Motor de Encriptación AES-256-CTR
 * 
 * Copiado directamente de: https://github.com/meshtastic/firmware/blob/master/src/CryptoEngine.h
 * Adaptado para ESP32-S3 con hardware acceleration
 */

#pragma once

#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

/**
 * Motor de encriptación basado en Meshtastic
 * Ver docs/encryption.md para detalles técnicos
 */
class CryptoEngine
{
  private:
    /// How many bytes in our private key
    uint8_t keySize = 0;

    /// The key we are encrypting/decrypting with
    uint8_t key[32];

    /// Our AES context for mbedtls
    mbedtls_aes_context aes;

    /// mbedtls entropy context for random number generation
    mbedtls_entropy_context entropy;

    /// mbedtls CTR_DRBG context for deterministic random bit generation
    mbedtls_ctr_drbg_context ctr_drbg;

    /// true if the crypto engine has been properly initialized
    bool initialized = false;

  public:
    CryptoEngine() {}

    ~CryptoEngine() {}

    /**
     * Set the key for encrypt and decrypt operations
     *
     * As a special case: if keySize is 0, then we assume no crypto should be done (null crypto)
     *
     * @param keySize can be 16 (AES128), 32 (AES256) or 0 (no crypt)
     * @param keyBytes a _static_ buffer that will remain valid for the life of this crypto instance (i.e. this class will cache the
     * provided pointer)
     */
    void setKey(size_t keySize, uint8_t *keyBytes);

    /**
     * Encrypt a packet
     *
     * @param bytes is updated in place
     */
    void encrypt(uint32_t fromNode, uint64_t packetNum, size_t numBytes, uint8_t *bytes);

    /**
     * Decrypt a packet. Returns the number of bytes decrypted
     *
     * @param bytes is updated in place
     * @return <0 for error, otherwise returns the number of bytes decrypted
     */
    int decrypt(uint32_t fromNode, uint64_t packetNum, size_t numBytes, uint8_t *bytes);

    /**
     * Return true if we think crypto should be used for this packet (either encrypting or decrypting).
     * 
     * NOTE: this is not a perfect check, it is possible to have false positives - i.e. packets
     * that we think are encrypted but are actually cleartext. This is because we are only
     * examining the packet header, not the payload.
     */
    bool isEncrypted(const uint8_t *bytes, size_t numBytes);

    /**
     * Initialize random number generator and other crypto primitives
     */
    void init();

    /**
     * Generate truly random bytes using ESP32 hardware RNG
     * @param buf Buffer to fill with random bytes
     * @param len Number of bytes to generate
     * @return 0 on success, negative on error
     */
    int random(uint8_t *buf, size_t len);

  private:
    /**
     * Init our 128 bit nonce for a new packet
     * 
     * @param packetNum the packet sequence number (allows the decrypt to work)
     * @param srcnode the sending node ID (allows the decrypt to work)
     * @param nonce Pointer to the 128-bit nonce that will be initialized
     */
    void initNonce(uint32_t fromNode, uint64_t packetNum, uint8_t *nonce);
};

/**
 * Global crypto engine instance
 */
extern CryptoEngine *crypto;