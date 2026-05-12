/**
 * Nova Native Standard Library - Cryptography Module
 */

#ifndef NOVA_STDLIB_CRYPTO_H
#define NOVA_STDLIB_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// HASHING
// ═══════════════════════════════════════════════════════════════════════════

// SHA-256 hash (returns 64-char hex string, caller frees)
char *nova_sha256(const uint8_t *data, size_t len);

// SHA-256 of string
char *nova_sha256_str(const char *str);

// Blake3 hash (returns 64-char hex string, caller frees)
char *nova_blake3(const uint8_t *data, size_t len);

// ═══════════════════════════════════════════════════════════════════════════
// SYMMETRIC ENCRYPTION (AES-256-GCM)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  uint8_t *data;
  size_t len;
  uint8_t nonce[12];
  uint8_t tag[16];
} NovaCiphertext;

// Generate random key (32 bytes for AES-256)
uint8_t *nova_generate_key(size_t len);

// Encrypt with AES-256-GCM
NovaCiphertext *nova_aes_encrypt(const uint8_t *key,
                                     const uint8_t *plaintext, size_t len);

// Decrypt with AES-256-GCM
uint8_t *nova_aes_decrypt(const uint8_t *key, NovaCiphertext *ct,
                            size_t *out_len);

void nova_ciphertext_free(NovaCiphertext *ct);

// ═══════════════════════════════════════════════════════════════════════════
// RANDOM
// ═══════════════════════════════════════════════════════════════════════════

// Cryptographically secure random bytes
void nova_random_bytes(uint8_t *buffer, size_t len);

// Random integer in range [min, max]
int64_t nova_random_int(int64_t min, int64_t max);

// Random UUID v4 (caller frees)
char *nova_uuid(void);

// ═══════════════════════════════════════════════════════════════════════════
// BASE64
// ═══════════════════════════════════════════════════════════════════════════

char *nova_base64_encode(const uint8_t *data, size_t len);
uint8_t *nova_base64_decode(const char *encoded, size_t *out_len);

#endif // NOVA_STDLIB_CRYPTO_H
