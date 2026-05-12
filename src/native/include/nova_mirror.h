/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_mirror.h - Zero-Copy Delta Encoding & Remote Reconstruction
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Core Idea (User's Innovation):
 *   Instead of sending data, send the RECIPE to reconstruct it.
 *   Instead of transferring full frames, send only what CHANGED.
 *
 * Supported Types:
 *   - FP32 tensors (AI inference)
 *   - UINT8 buffers (images, video frames)
 *   - INT16 buffers (audio samples)
 *   - Raw bytes (files, arbitrary data)
 *
 * Usage Modes:
 *   MODE 1: Delta Patch    → Send only changed bytes
 *   MODE 2: Mirror Recipe  → Send reconstruction instructions
 *   MODE 3: Hybrid         → Choose optimal strategy per block
 */

#ifndef NOVA_MIRROR_H
#define NOVA_MIRROR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// DATA TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  ZMIRROR_TYPE_FP32,  // AI tensors, model weights
  ZMIRROR_TYPE_UINT8, // Images (PNG/JPEG pixels), video frames
  ZMIRROR_TYPE_INT16, // Audio samples (PCM)
  ZMIRROR_TYPE_RAW,   // Files, arbitrary binary data
} ZMirrorDataType;

typedef enum {
  ZMIRROR_STRATEGY_AUTO,      // Let engine decide
  ZMIRROR_STRATEGY_FULL,      // Send everything (baseline)
  ZMIRROR_STRATEGY_DELTA,     // Send only differences
  ZMIRROR_STRATEGY_RECIPE,    // Send reconstruction instructions
  ZMIRROR_STRATEGY_RLE_DELTA, // Run-length encoded delta (sparse changes)
} ZMirrorStrategy;

// ═══════════════════════════════════════════════════════════════════════════
// DELTA PATCH: Compact representation of changes
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  uint32_t offset; // Where the change starts (byte offset)
  uint32_t length; // How many bytes changed
  // Followed by `length` bytes of new data (flexible array)
} ZMirrorPatchRegion;

typedef struct {
  ZMirrorDataType type;
  ZMirrorStrategy strategy_used;

  uint64_t original_size;   // Size of full data
  uint64_t patch_size;      // Size of this patch (what we actually send)
  double compression_ratio; // original_size / patch_size
  double similarity;        // 0.0 - 1.0, how similar old vs new

  uint32_t num_regions;        // Number of changed regions
  ZMirrorPatchRegion *regions; // Array of changed regions
  uint8_t *patch_data;         // Concatenated patch data
  uint32_t patch_data_size;    // Total bytes in patch_data
} ZMirrorPatch;

// ═══════════════════════════════════════════════════════════════════════════
// MIRROR RECIPE: Reconstruction instructions (not data!)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  ZMIRROR_OP_COPY_BASE,  // Copy from base state
  ZMIRROR_OP_FILL,       // Fill region with constant
  ZMIRROR_OP_ADD_SCALAR, // Add scalar to region
  ZMIRROR_OP_MUL_SCALAR, // Multiply region by scalar
  ZMIRROR_OP_PATCH_RAW,  // Apply raw bytes
  ZMIRROR_OP_XOR_PATCH,  // XOR with patch (for binary files)
} ZMirrorOpType;

typedef struct {
  ZMirrorOpType op;
  uint32_t offset;
  uint32_t length;
  union {
    float scalar_f32;
    uint8_t fill_byte;
    uint32_t patch_offset; // Offset into recipe's data buffer
  };
} ZMirrorInstruction;

typedef struct {
  uint32_t num_instructions;
  ZMirrorInstruction *instructions;
  uint8_t *data_buffer; // Shared data for PATCH_RAW ops
  uint32_t data_buffer_size;
  uint64_t original_size;
  double compression_ratio;
} ZMirrorRecipe;

// ═══════════════════════════════════════════════════════════════════════════
// MIRROR STATE: Tracks "base" state for delta computation
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  void *base_data;    // Previous state (kept for delta computation)
  uint64_t base_size; // Size of base data
  ZMirrorDataType type;
  uint64_t frame_count;          // How many frames processed
  uint64_t total_bytes_saved;    // Cumulative savings
  uint64_t total_bytes_original; // What we WOULD have sent

  // Per-type thresholds
  float fp32_threshold;    // Min change to count as "different" for fp32
  uint8_t uint8_threshold; // Min pixel change to count
  int16_t int16_threshold; // Min sample change to count
} ZMirrorState;

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════

// Create/destroy mirror state
ZMirrorState *zmirror_create(ZMirrorDataType type, uint64_t data_size);
void zmirror_destroy(ZMirrorState *state);

// Set thresholds (what counts as "changed")
void zmirror_set_threshold_fp32(ZMirrorState *state, float threshold);
void zmirror_set_threshold_uint8(ZMirrorState *state, uint8_t threshold);
void zmirror_set_threshold_int16(ZMirrorState *state, int16_t threshold);

// Compute delta patch between current state and new data
ZMirrorPatch *zmirror_compute_patch(ZMirrorState *state, const void *new_data,
                                    uint64_t new_size);

// Apply patch to reconstruct new data (receiver side)
void zmirror_apply_patch(const ZMirrorPatch *patch,
                         void *base_data, // in-place update
                         uint64_t base_size);

// Update base state (call after successful transfer)
void zmirror_update_base(ZMirrorState *state, const void *new_data,
                         uint64_t size);

// Free patch
void zmirror_free_patch(ZMirrorPatch *patch);

// Stats
void zmirror_print_stats(const ZMirrorState *state);

#endif // NOVA_MIRROR_H
