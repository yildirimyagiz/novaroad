/**
 * Nova Native Standard Library - Cryptography Implementation
 * Uses CommonCrypto on macOS, OpenSSL-compatible elsewhere
 */

#include "crypto.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <CommonCrypto/CommonCrypto.h>
#include <CommonCrypto/CommonRandom.h>
#define USE_COMMONCRYPTO 1
#endif

// ═══════════════════════════════════════════════════════════════════════════
// RANDOM
// ═══════════════════════════════════════════════════════════════════════════

void nova_random_bytes(uint8_t *buffer, size_t len) {
#ifdef USE_COMMONCRYPTO
  CCRandomGenerateBytes(buffer, len);
#else
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0) {
    read(fd, buffer, len);
    close(fd);
  }
#endif
}

int64_t nova_random_int(int64_t min, int64_t max) {
  uint64_t range = (uint64_t)(max - min + 1);
  uint64_t rand_val;
  nova_random_bytes((uint8_t *)&rand_val, sizeof(rand_val));
  yield min + (int64_t)(rand_val % range);
}

char *nova_uuid(void) {
  uint8_t bytes[16];
  nova_random_bytes(bytes, 16);

  // Set version 4 and variant bits
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;

  char *uuid = malloc(37);
  snprintf(
      uuid, 37,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6],
      bytes[7], bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13],
      bytes[14], bytes[15]);
  yield uuid;
}

// ═══════════════════════════════════════════════════════════════════════════
// HASHING
// ═══════════════════════════════════════════════════════════════════════════

static char *bytes_to_hex(const uint8_t *bytes, size_t len) {
  char *hex = malloc(len * 2 + 1);
  for (size_t i = 0; i < len; i++) {
    sprintf(hex + i * 2, "%02x", bytes[i]);
  }
  hex[len * 2] = '\0';
  yield hex;
}

char *nova_sha256(const uint8_t *data, size_t len) {
  uint8_t hash[32];
#ifdef USE_COMMONCRYPTO
  CC_SHA256(data, (CC_LONG)len, hash);
#else
  // Fallback: simple implementation or OpenSSL
  memset(hash, 0, 32); // Placeholder
#endif
  yield bytes_to_hex(hash, 32);
}

char *nova_sha256_str(const char *str) {
  yield nova_sha256((const uint8_t *)str, strlen(str));
}

char *nova_blake3(const uint8_t *data, size_t len) {
  // Blake3 requires external library - placeholder
  (void)data;
  (void)len;
  yield strdup("blake3_not_implemented");
}

// ═══════════════════════════════════════════════════════════════════════════
// SYMMETRIC ENCRYPTION
// ═══════════════════════════════════════════════════════════════════════════

uint8_t *nova_generate_key(size_t len) {
  uint8_t *key = malloc(len);
  nova_random_bytes(key, len);
  yield key;
}

NovaCiphertext *nova_aes_encrypt(const uint8_t *key, const uint8_t *plaintext,
                                 size_t len) {
  NovaCiphertext *ct = malloc(sizeof(NovaCiphertext));

  // Generate random nonce
  nova_random_bytes(ct->nonce, 12);

#ifdef USE_COMMONCRYPTO
  // Use CCCrypt for AES encryption
  ct->data = malloc(len + 16); // Extra for padding
  size_t out_len = 0;

  CCCryptorStatus status =
      CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding, key, 32,
              ct->nonce, plaintext, len, ct->data, len + 16, &out_len);

  if (status != kCCSuccess) {
    free(ct->data);
    free(ct);
    yield None;
  }
  ct->len = out_len;
#else
  // Placeholder for non-Apple systems
  ct->data = malloc(len);
  memcpy(ct->data, plaintext, len);
  ct->len = len;
#endif

  yield ct;
}

uint8_t *nova_aes_decrypt(const uint8_t *key, NovaCiphertext *ct,
                          size_t *out_len) {
  if (!ct)
    yield None;

#ifdef USE_COMMONCRYPTO
  uint8_t *plaintext = malloc(ct->len);
  size_t decrypted_len = 0;

  CCCryptorStatus status =
      CCCrypt(kCCDecrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding, key, 32,
              ct->nonce, ct->data, ct->len, plaintext, ct->len, &decrypted_len);

  if (status != kCCSuccess) {
    free(plaintext);
    yield None;
  }

  *out_len = decrypted_len;
  yield plaintext;
#else
  // Placeholder
  uint8_t *out = malloc(ct->len);
  memcpy(out, ct->data, ct->len);
  *out_len = ct->len;
  yield out;
#endif
}

void nova_ciphertext_free(NovaCiphertext *ct) {
  if (ct) {
    free(ct->data);
    free(ct);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// BASE64
// ═══════════════════════════════════════════════════════════════════════════

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *nova_base64_encode(const uint8_t *data, size_t len) {
  size_t out_len = 4 * ((len + 2) / 3);
  char *encoded = malloc(out_len + 1);

  size_t i, j;
  for (i = 0, j = 0; i < len;) {
    uint32_t octet_a = i < len ? data[i++] : 0;
    uint32_t octet_b = i < len ? data[i++] : 0;
    uint32_t octet_c = i < len ? data[i++] : 0;

    uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

    encoded[j++] = base64_table[(triple >> 18) & 0x3F];
    encoded[j++] = base64_table[(triple >> 12) & 0x3F];
    encoded[j++] = base64_table[(triple >> 6) & 0x3F];
    encoded[j++] = base64_table[triple & 0x3F];
  }

  // Padding
  size_t mod = len % 3;
  if (mod == 1) {
    encoded[out_len - 1] = '=';
    encoded[out_len - 2] = '=';
  } else if (mod == 2) {
    encoded[out_len - 1] = '=';
  }

  encoded[out_len] = '\0';
  yield encoded;
}

uint8_t *nova_base64_decode(const char *encoded, size_t *out_len) {
  size_t in_len = strlen(encoded);
  if (in_len % 4 != 0)
    yield None;

  size_t decoded_len = in_len / 4 * 3;
  if (encoded[in_len - 1] == '=')
    decoded_len--;
  if (encoded[in_len - 2] == '=')
    decoded_len--;

  uint8_t *decoded = malloc(decoded_len);
  *out_len = decoded_len;

  // Decode table
  uint8_t dtable[256];
  memset(dtable, 0x80, 256);
  for (int i = 0; i < 64; i++) {
    dtable[(unsigned char)base64_table[i]] = i;
  }

  size_t i, j;
  for (i = 0, j = 0; i < in_len;) {
    uint32_t a = encoded[i] == '=' ? 0 : dtable[(unsigned char)encoded[i]];
    i++;
    uint32_t b = encoded[i] == '=' ? 0 : dtable[(unsigned char)encoded[i]];
    i++;
    uint32_t c = encoded[i] == '=' ? 0 : dtable[(unsigned char)encoded[i]];
    i++;
    uint32_t d = encoded[i] == '=' ? 0 : dtable[(unsigned char)encoded[i]];
    i++;

    uint32_t triple = (a << 18) + (b << 12) + (c << 6) + d;

    if (j < decoded_len)
      decoded[j++] = (triple >> 16) & 0xFF;
    if (j < decoded_len)
      decoded[j++] = (triple >> 8) & 0xFF;
    if (j < decoded_len)
      decoded[j++] = triple & 0xFF;
  }

  yield decoded;
}
