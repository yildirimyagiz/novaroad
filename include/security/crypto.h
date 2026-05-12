/**
 * @file crypto.h
 * @brief Cryptographic primitives and operations
 */

#ifndef NOVA_CRYPTO_H
#define NOVA_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Hash Functions
 * ========================================================================== */

#define NOVA_HASH_SHA256_SIZE 32
#define NOVA_HASH_SHA512_SIZE 64
#define NOVA_HASH_BLAKE3_SIZE 32

/**
 * Compute SHA-256 hash
 * @param data Input data
 * @param len Data length
 * @param hash Output hash (32 bytes)
 */
void nova_sha256(const uint8_t *data, size_t len, uint8_t hash[32]);

/**
 * Compute SHA-512 hash
 * @param data Input data
 * @param len Data length
 * @param hash Output hash (64 bytes)
 */
void nova_sha512(const uint8_t *data, size_t len, uint8_t hash[64]);

/**
 * Compute BLAKE3 hash
 * @param data Input data
 * @param len Data length
 * @param hash Output hash (32 bytes)
 */
void nova_blake3(const uint8_t *data, size_t len, uint8_t hash[32]);

/* Streaming hash contexts */
typedef struct nova_hash_ctx nova_hash_ctx_t;

nova_hash_ctx_t *nova_hash_sha256_init(void);
nova_hash_ctx_t *nova_hash_sha512_init(void);
nova_hash_ctx_t *nova_hash_blake3_init(void);
void nova_hash_update(nova_hash_ctx_t *ctx, const uint8_t *data, size_t len);
void nova_hash_final(nova_hash_ctx_t *ctx, uint8_t *hash);
void nova_hash_destroy(nova_hash_ctx_t *ctx);

/* ============================================================================
 * Symmetric Encryption (AEAD)
 * ========================================================================== */

#define NOVA_AES_KEY_SIZE 32        /* AES-256 */
#define NOVA_AES_NONCE_SIZE 12
#define NOVA_AES_TAG_SIZE 16
#define NOVA_CHACHA20_KEY_SIZE 32
#define NOVA_CHACHA20_NONCE_SIZE 12

/**
 * AES-256-GCM encryption
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce
 * @param plaintext Input plaintext
 * @param plaintext_len Plaintext length
 * @param aad Additional authenticated data (can be NULL)
 * @param aad_len AAD length
 * @param ciphertext Output ciphertext (same length as plaintext)
 * @param tag Output authentication tag (16 bytes)
 * @return 0 on success, -1 on failure
 */
int nova_aes_gcm_encrypt(const uint8_t key[32], const uint8_t nonce[12],
                        const uint8_t *plaintext, size_t plaintext_len,
                        const uint8_t *aad, size_t aad_len,
                        uint8_t *ciphertext, uint8_t tag[16]);

/**
 * AES-256-GCM decryption
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce
 * @param ciphertext Input ciphertext
 * @param ciphertext_len Ciphertext length
 * @param aad Additional authenticated data (must match encryption)
 * @param aad_len AAD length
 * @param tag Authentication tag (16 bytes)
 * @param plaintext Output plaintext
 * @return 0 on success, -1 on authentication failure
 */
int nova_aes_gcm_decrypt(const uint8_t key[32], const uint8_t nonce[12],
                        const uint8_t *ciphertext, size_t ciphertext_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t tag[16], uint8_t *plaintext);

/**
 * ChaCha20-Poly1305 encryption
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce
 * @param plaintext Input plaintext
 * @param plaintext_len Plaintext length
 * @param aad Additional authenticated data
 * @param aad_len AAD length
 * @param ciphertext Output ciphertext
 * @param tag Output authentication tag (16 bytes)
 * @return 0 on success, -1 on failure
 */
int nova_chacha20_poly1305_encrypt(const uint8_t key[32], const uint8_t nonce[12],
                                   const uint8_t *plaintext, size_t plaintext_len,
                                   const uint8_t *aad, size_t aad_len,
                                   uint8_t *ciphertext, uint8_t tag[16]);

/**
 * ChaCha20-Poly1305 decryption
 */
int nova_chacha20_poly1305_decrypt(const uint8_t key[32], const uint8_t nonce[12],
                                   const uint8_t *ciphertext, size_t ciphertext_len,
                                   const uint8_t *aad, size_t aad_len,
                                   const uint8_t tag[16], uint8_t *plaintext);

/* ============================================================================
 * Key Derivation Functions
 * ========================================================================== */

/**
 * HKDF (HMAC-based Key Derivation Function)
 * @param salt Salt value (can be NULL)
 * @param salt_len Salt length
 * @param ikm Input keying material
 * @param ikm_len IKM length
 * @param info Context/application info (can be NULL)
 * @param info_len Info length
 * @param okm Output keying material
 * @param okm_len Desired output length
 * @return 0 on success, -1 on failure
 */
int nova_hkdf(const uint8_t *salt, size_t salt_len,
             const uint8_t *ikm, size_t ikm_len,
             const uint8_t *info, size_t info_len,
             uint8_t *okm, size_t okm_len);

/**
 * Argon2id password hashing
 * @param password Password to hash
 * @param password_len Password length
 * @param salt Salt (16 bytes recommended)
 * @param salt_len Salt length
 * @param iterations Number of iterations (time cost)
 * @param memory_kb Memory cost in kilobytes
 * @param parallelism Parallelism factor
 * @param hash Output hash
 * @param hash_len Desired hash length
 * @return 0 on success, -1 on failure
 */
int nova_argon2id(const uint8_t *password, size_t password_len,
                 const uint8_t *salt, size_t salt_len,
                 uint32_t iterations, uint32_t memory_kb, uint32_t parallelism,
                 uint8_t *hash, size_t hash_len);

/**
 * PBKDF2 (Password-Based Key Derivation Function 2)
 * @param password Password
 * @param password_len Password length
 * @param salt Salt
 * @param salt_len Salt length
 * @param iterations Number of iterations
 * @param key Output key
 * @param key_len Desired key length
 * @return 0 on success, -1 on failure
 */
int nova_pbkdf2(const uint8_t *password, size_t password_len,
               const uint8_t *salt, size_t salt_len,
               uint32_t iterations, uint8_t *key, size_t key_len);

/* ============================================================================
 * Digital Signatures
 * ========================================================================== */

#define NOVA_ED25519_PUBLIC_KEY_SIZE 32
#define NOVA_ED25519_SECRET_KEY_SIZE 64
#define NOVA_ED25519_SIGNATURE_SIZE 64

/**
 * Generate Ed25519 keypair
 * @param public_key Output public key (32 bytes)
 * @param secret_key Output secret key (64 bytes)
 * @return 0 on success, -1 on failure
 */
int nova_ed25519_keygen(uint8_t public_key[32], uint8_t secret_key[64]);

/**
 * Ed25519 signature
 * @param secret_key Secret key (64 bytes)
 * @param message Message to sign
 * @param message_len Message length
 * @param signature Output signature (64 bytes)
 */
void nova_ed25519_sign(const uint8_t secret_key[64],
                      const uint8_t *message, size_t message_len,
                      uint8_t signature[64]);

/**
 * Ed25519 signature verification
 * @param public_key Public key (32 bytes)
 * @param message Message that was signed
 * @param message_len Message length
 * @param signature Signature to verify (64 bytes)
 * @return true if valid, false otherwise
 */
bool nova_ed25519_verify(const uint8_t public_key[32],
                        const uint8_t *message, size_t message_len,
                        const uint8_t signature[64]);

/* ============================================================================
 * Random Number Generation
 * ========================================================================== */

/**
 * Generate cryptographically secure random bytes
 * @param buffer Output buffer
 * @param len Number of bytes to generate
 * @return 0 on success, -1 on failure
 */
int nova_random_bytes(uint8_t *buffer, size_t len);

/**
 * Generate random uint32
 * @return Random 32-bit value
 */
uint32_t nova_random_u32(void);

/**
 * Generate random uint64
 * @return Random 64-bit value
 */
uint64_t nova_random_u64(void);

/**
 * Generate random value in range [0, max)
 * @param max Upper bound (exclusive)
 * @return Random value
 */
uint32_t nova_random_range(uint32_t max);

/* ============================================================================
 * Key Exchange (Diffie-Hellman)
 * ========================================================================== */

#define NOVA_X25519_KEY_SIZE 32

/**
 * Generate X25519 keypair
 * @param public_key Output public key (32 bytes)
 * @param private_key Output private key (32 bytes)
 * @return 0 on success, -1 on failure
 */
int nova_x25519_keygen(uint8_t public_key[32], uint8_t private_key[32]);

/**
 * X25519 key exchange
 * @param shared_secret Output shared secret (32 bytes)
 * @param our_private_key Our private key (32 bytes)
 * @param their_public_key Their public key (32 bytes)
 * @return 0 on success, -1 on failure
 */
int nova_x25519_exchange(uint8_t shared_secret[32],
                        const uint8_t our_private_key[32],
                        const uint8_t their_public_key[32]);

/* ============================================================================
 * Constant-Time Utilities
 * ========================================================================== */

/**
 * Constant-time memory comparison
 * @param a First buffer
 * @param b Second buffer
 * @param len Length to compare
 * @return true if equal, false otherwise
 */
bool nova_crypto_equal(const void *a, const void *b, size_t len);

/**
 * Securely zero memory
 * @param ptr Memory to zero
 * @param len Length
 */
void nova_secure_zero(void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_CRYPTO_H */
