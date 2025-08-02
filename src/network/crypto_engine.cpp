/*
 * CRYPTO_ENGINE.CPP - Implementación del Motor de Encriptación AES-256-CTR
 * 
 * Copiado directamente de: https://github.com/meshtastic/firmware/blob/master/src/CryptoEngine.cpp
 * Adaptado para ESP32-S3 con hardware acceleration
 */

#include "crypto_engine.h"
#include <esp_random.h>
#include <mbedtls/platform.h>

// Global crypto engine instance
CryptoEngine *crypto = nullptr;

void CryptoEngine::setKey(size_t keySize, uint8_t *keyBytes)
{
    Serial.printf("[CRYPTO] Installing AES%d key\n", keySize * 8);
    this->keySize = keySize;
    
    if (keySize == 0) {
        // Null crypto - no encryption
        this->key[0] = 0;
        Serial.println("[CRYPTO] Crypto disabled (null key)");
        return;
    }
    
    if (keySize != 16 && keySize != 32) {
        Serial.printf("[CRYPTO] Invalid key size: %d (must be 16 or 32)\n", keySize);
        return;
    }
    
    // Copy key to our buffer
    memcpy(this->key, keyBytes, keySize);
    
    // Initialize mbedtls AES context with our key
    int ret = mbedtls_aes_setkey_enc(&aes, this->key, keySize * 8);
    if (ret != 0) {
        Serial.printf("[CRYPTO] AES key setup failed: %d\n", ret);
        return;
    }
    
    Serial.printf("[CRYPTO] Crypto engine configured with AES%d\n", keySize * 8);
}

void CryptoEngine::encrypt(uint32_t fromNode, uint64_t packetNum, size_t numBytes, uint8_t *bytes)
{
    if (keySize == 0) {
        // Null crypto - do nothing
        return;
    }
    
    if (numBytes == 0) {
        Serial.println("[CRYPTO] Warning: Attempting to encrypt zero bytes");
        return;
    }
    
    // Initialize nonce for this packet
    uint8_t nonce[16];
    initNonce(fromNode, packetNum, nonce);
    
    // AES-CTR encryption using mbedtls
    size_t nc_off = 0;
    uint8_t stream_block[16];
    
    int ret = mbedtls_aes_crypt_ctr(&aes, numBytes, &nc_off, nonce, stream_block, bytes, bytes);
    
    if (ret != 0) {
        Serial.printf("[CRYPTO] AES encryption failed: %d\n", ret);
        return;
    }
    
    Serial.printf("[CRYPTO] Encrypted %d bytes for node %08x, packet %llu\n", numBytes, fromNode, packetNum);
}

int CryptoEngine::decrypt(uint32_t fromNode, uint64_t packetNum, size_t numBytes, uint8_t *bytes)
{
    if (keySize == 0) {
        // Null crypto - nothing to decrypt
        return numBytes;
    }
    
    if (numBytes == 0) {
        Serial.println("[CRYPTO] Warning: Attempting to decrypt zero bytes");
        return 0;
    }
    
    // Initialize nonce for this packet
    uint8_t nonce[16];
    initNonce(fromNode, packetNum, nonce);
    
    // AES-CTR decryption (same as encryption in CTR mode)
    size_t nc_off = 0;
    uint8_t stream_block[16];
    
    int ret = mbedtls_aes_crypt_ctr(&aes, numBytes, &nc_off, nonce, stream_block, bytes, bytes);
    
    if (ret != 0) {
        Serial.printf("[CRYPTO] AES decryption failed: %d\n", ret);
        return -1;
    }
    
    Serial.printf("[CRYPTO] Decrypted %d bytes from node %08x, packet %llu\n", numBytes, fromNode, packetNum);
    
    return numBytes;
}

bool CryptoEngine::isEncrypted(const uint8_t *bytes, size_t numBytes)
{
    if (keySize == 0 || numBytes == 0) {
        return false;
    }
    
    // Simple heuristic: encrypted packets typically have high entropy
    // This is not foolproof but works for basic detection
    
    // Count byte frequency distribution
    uint8_t freq[256] = {0};
    for (size_t i = 0; i < numBytes && i < 64; i++) { // Sample first 64 bytes
        freq[bytes[i]]++;
    }
    
    // Calculate entropy indicator
    int nonZeroBytes = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) nonZeroBytes++;
    }
    
    // High byte diversity suggests encryption
    bool highEntropy = (nonZeroBytes > (numBytes / 4));
    
    Serial.printf("[CRYPTO] Packet entropy check: %d/%d unique bytes, encrypted=%s\n", 
              nonZeroBytes, (int)numBytes, highEntropy ? "yes" : "no");
    
    return highEntropy;
}

void CryptoEngine::init()
{
    if (initialized) {
        return;
    }
    
    Serial.println("[CRYPTO] Initializing crypto engine...");
    
    // Initialize mbedtls contexts
    mbedtls_aes_init(&aes);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    // Seed the random number generator
    const char *pers = "meshtastic_crypto";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char *)pers, strlen(pers));
    
    if (ret != 0) {
        Serial.printf("[CRYPTO] Failed to seed RNG: %d\n", ret);
        return;
    }
    
    // Add ESP32 hardware entropy
    uint8_t hw_entropy[32];
    esp_fill_random(hw_entropy, sizeof(hw_entropy));
    mbedtls_entropy_update_manual(&entropy, hw_entropy, sizeof(hw_entropy));
    
    initialized = true;
    Serial.println("[CRYPTO] Crypto engine initialized successfully");
}

int CryptoEngine::random(uint8_t *buf, size_t len)
{
    if (!initialized) {
        init();
    }
    
    if (!buf || len == 0) {
        return -1;
    }
    
    // Use ESP32 hardware RNG for best entropy
    esp_fill_random(buf, len);
    
    // Additional mixing with mbedtls DRBG for extra security
    uint8_t drbg_output[32];
    if (len <= sizeof(drbg_output)) {
        int ret = mbedtls_ctr_drbg_random(&ctr_drbg, drbg_output, len);
        if (ret == 0) {
            // XOR hardware RNG with DRBG output
            for (size_t i = 0; i < len; i++) {
                buf[i] ^= drbg_output[i];
            }
        }
    }
    
    Serial.printf("[CRYPTO] Generated %d random bytes\n", len);
    return 0;
}

void CryptoEngine::initNonce(uint32_t fromNode, uint64_t packetNum, uint8_t *nonce)
{
    // Nonce structure copiada exactamente de Meshtastic:
    // Bytes 0-3: fromNode (little endian)
    // Bytes 4-11: packetNum (little endian) 
    // Bytes 12-15: zeros (for CTR mode)
    
    memset(nonce, 0, 16);
    
    // Pack fromNode (32-bit) into bytes 0-3
    nonce[0] = (fromNode >> 0) & 0xff;
    nonce[1] = (fromNode >> 8) & 0xff;
    nonce[2] = (fromNode >> 16) & 0xff;
    nonce[3] = (fromNode >> 24) & 0xff;
    
    // Pack packetNum (64-bit) into bytes 4-11
    nonce[4] = (packetNum >> 0) & 0xff;
    nonce[5] = (packetNum >> 8) & 0xff;
    nonce[6] = (packetNum >> 16) & 0xff;
    nonce[7] = (packetNum >> 24) & 0xff;
    nonce[8] = (packetNum >> 32) & 0xff;
    nonce[9] = (packetNum >> 40) & 0xff;
    nonce[10] = (packetNum >> 48) & 0xff;
    nonce[11] = (packetNum >> 56) & 0xff;
    
    // Bytes 12-15 remain zero (CTR counter starts at 0)
    
    Serial.printf("[CRYPTO] Initialized nonce for node %08x, packet %llu\n", fromNode, packetNum);
}