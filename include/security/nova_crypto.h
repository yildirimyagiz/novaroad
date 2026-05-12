/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_crypto.h - Sovereign Cryptographic Kernels
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_CRYPTO_H
#define NOVA_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

// SHA-256 Context Structure
typedef struct {
  uint32_t state[8];
  uint64_t bitlen;
  uint8_t data[64];
  uint32_t datalen;
} NovaSHA256Ctx;

// Midstate for Mining (Delta Optimization)
typedef struct {
  uint32_t state[8];
} NovaSHA256Midstate;

// Standard SHA-256
void nova_sha256_init(NovaSHA256Ctx *ctx);
void nova_sha256_update(NovaSHA256Ctx *ctx, const uint8_t *data,
                          size_t len);
void nova_sha256_final(NovaSHA256Ctx *ctx, uint8_t *hash);

// Delta Mining API
NovaSHA256Midstate nova_crypto_precompute_midstate(const uint8_t *block_64);
void nova_crypto_sha256_mine_delta(const NovaSHA256Midstate *mid,
                                     uint32_t nonce, uint8_t *out_hash);

// Parallel Sovereign Crypto
void nova_crypto_sha256_parallel_neon(const uint8_t *header,
                                        uint32_t start_nonce, int N,
                                        uint32_t *out_hashes);

// ─────────────────────────────────────────────────────────────────────────────
// SOVEREIGN SECURITY LAYER (Post-Quantum & Formal Verification)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @proof: Isabelle/HOL Theory "Nova_Kyber_Safety"
 * Implements Kyber-768 (PQC KEM)
 */
typedef struct {
  uint8_t pk[1184];
  uint8_t sk[2400];
} NovaPQCKeypair;

int nova_pqc_kyber_generate(NovaPQCKeypair *keys);
int nova_pqc_kyber_encapsulate(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int nova_pqc_kyber_decapsulate(uint8_t *ss, const uint8_t *ct,
                                 const uint8_t *sk);

/**
 * @proof: Isabelle/HOL Theory "Nova_Dilithium_Auth"
 * Implements Dilithium-3 (PQC Signature)
 */
int nova_pqc_dilithium_sign(uint8_t *sig, const uint8_t *m, size_t mlen,
                              const uint8_t *sk);
int nova_pqc_dilithium_verify(const uint8_t *sig, const uint8_t *m,
                                size_t mlen, const uint8_t *pk);

// ─────────────────────────────────────────────────────────────────────────────
// ACTIVE COUNTER-STRIKE (Martial Arts Defense System)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
  NOVA_STRIKE_JUDO,      // Use attacker force (Packet Mirroring/Redirection)
  NOVA_STRIKE_TAEKWONDO, // Rapid high-impact strike (Resource
                           // Exhaustion/Blocking)
  NOVA_STRIKE_AIKIDO,    // Redirect to sandbox/C2 (Payload Inversion)
  NOVA_STRIKE_KARATE // Direct neutralization (Session Hijack/Counter-Exploit)
} NovaCounterStrikeMode;

typedef struct {
  char attacker_ip[64];
  uint16_t vulnerability_score;
  char identified_tool[64];
} NovaAttackerIntel;

/**
 * Traces the attacker and prepares for retaliatory action.
 */
NovaAttackerIntel nova_crypto_trace_attacker(const uint8_t *access_token);

/**
 * Initiates an active counter-strike following a detected breach attempt.
 * Links directly to the Sovereign Martial Arts Defense Engine.
 */
void nova_attack_counter_strike(const NovaAttackerIntel *intel,
                                  NovaCounterStrikeMode mode);

/**
 * High-speed X25519 Key Exchange
 */
void nova_crypto_x25519(uint8_t *shared_secret, const uint8_t *priv,
                          const uint8_t *pub);

// Zero-Knowledge Proof (ZKP) Primitives for Privacy-Preserving Gov Systems
typedef struct {
  uint8_t proof_data[256];
} NovaZKPPayload;

int nova_zkp_verify_identity(const NovaZKPPayload *p,
                               const uint8_t *public_input);

// ─────────────────────────────────────────────────────────────────────────────
// DECEPTIVE DEFENSE (Honey Encryption)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Decrypts data using the sovereign protocol.
 * If 'is_authorized' is false, serves decoy intel to the attacker.
 * @proof: Isabelle/HOL Theory "Nova_Honey_Decoy_Invariant"
 */
void nova_crypto_decrypt_sovereign(uint8_t *output, const uint8_t *ciphertext,
                                     size_t len, const uint8_t *key,
                                     int is_authorized);

#endif // NOVA_CRYPTO_H
