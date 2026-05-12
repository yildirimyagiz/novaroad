#ifndef NOVA_CLIP_TOKENIZER_H
#define NOVA_CLIP_TOKENIZER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// PRODUCTION-GRADE CLIP BPE TOKENIZER
// ============================================================================
// Based on OpenAI CLIP tokenizer with BPE encoding
// Production-ready for Stable Diffusion inference

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

typedef struct NovaArena NovaArena; // Forward declaration

typedef struct {
    void* data;
    size_t size;
    size_t capacity;
    NovaArena* arena;
} NovaScratchBuffer;

// Arena-based allocation for ML workloads
struct NovaArena {
    void* base;
    size_t size;
    size_t used;
    NovaScratchBuffer* scratch_buffers;
    int num_scratch;
    int max_scratch;
};

// ============================================================================
// STRING HASH MAP (Production Requirement)
// ============================================================================

#define HASH_MAP_INITIAL_CAP 4096
#define HASH_MAP_LOAD_FACTOR 0.75f

typedef struct {
    char** keys;        // String keys
    int* values;        // Token IDs
    int* hashes;        // Precomputed hashes
    bool* occupied;     // Occupancy flags
    int capacity;       // Current capacity
    int size;          // Number of entries
    NovaArena* arena;
} NovaHashMap;

// Core hash map operations
NovaHashMap* nova_hashmap_create(NovaArena* arena, int initial_cap);
int nova_hashmap_get(NovaHashMap* map, const char* key, int* value);
int nova_hashmap_put(NovaHashMap* map, const char* key, int value);
void nova_hashmap_free(NovaHashMap* map);

// ============================================================================
// BPE MERGE SYSTEM (Production Requirement)
// ============================================================================

typedef struct {
    char** merge_a;     // First token in merge pair
    char** merge_b;     // Second token in merge pair
    int* ranks;         // Merge priority ranks
    int num_merges;     // Number of merge rules
    NovaHashMap* merge_map; // Fast lookup: pair -> rank
    NovaArena* arena;
} BPEMergeTable;

// BPE merge operations
BPEMergeTable* bpe_merge_table_create(NovaArena* arena, int num_merges);
int bpe_get_merge_rank(BPEMergeTable* table, const char* token_a, const char* token_b);
void bpe_merge_table_free(BPEMergeTable* table);

// ============================================================================
// BYTE ENCODER (Production Requirement)
// ============================================================================

typedef struct {
    unsigned char bytes[256][4];  // Byte to UTF-8 mapping
    int lengths[256];             // UTF-8 sequence lengths
    NovaHashMap* byte_map;      // Fast byte lookup
    NovaArena* arena;
} ByteEncoder;

// Byte encoding operations
ByteEncoder* byte_encoder_create(NovaArena* arena);
int byte_encoder_encode(ByteEncoder* enc, const char* text, int** tokens, int* num_tokens);
char* byte_encoder_decode(ByteEncoder* enc, const int* tokens, int num_tokens);
void byte_encoder_free(ByteEncoder* enc);

// ============================================================================
// BPE CACHE (Performance Optimization)
// ============================================================================

#define BPE_CACHE_SIZE 16384
#define BPE_CACHE_MASK (BPE_CACHE_SIZE - 1)

typedef struct {
    char** keys;        // Cached input strings
    int** values;       // Cached token sequences
    int* lengths;       // Token sequence lengths
    uint64_t* hashes;   // String hashes
    int size;          // Current cache size
    NovaArena* arena;
} BPECache;

// Cache operations
BPECache* bpe_cache_create(NovaArena* arena);
int bpe_cache_get(BPECache* cache, const char* text, uint64_t hash, int** tokens, int* length);
void bpe_cache_put(BPECache* cache, const char* text, uint64_t hash, const int* tokens, int length);
void bpe_cache_free(BPECache* cache);

// ============================================================================
// UNICODE NORMALIZATION (Production Requirement)
// ============================================================================

typedef struct {
    uint32_t* codepoints;    // Unicode codepoints
    int length;             // Number of codepoints
    NovaArena* arena;
} UnicodeString;

// Unicode operations
UnicodeString* unicode_normalize_nfkc(const char* input, NovaArena* arena);
char* unicode_to_utf8(const UnicodeString* str, NovaArena* arena);
void unicode_free(UnicodeString* str);

// ============================================================================
// REGEX PRE-TOKENIZATION (Production Requirement)
// ============================================================================

typedef struct {
    const char* pattern;     // Regex pattern string
    int* groups;            // Capture group indices
    int num_groups;         // Number of groups
} RegexPattern;

// Regex pre-tokenization
char** regex_split(const char* text, RegexPattern* pattern, int* num_parts, NovaArena* arena);

// ============================================================================
// PRODUCTION-GRADE CLIP TOKENIZER
// ============================================================================

typedef struct CLIPTokenizer {
    // Core vocabularies - FIXED: Real string hash maps
    NovaHashMap* vocab_map;           // ✅ String → Token ID mapping (FIXED!)
    BPEMergeTable* merges;              // ✅ Pair → Rank table (FIXED!)
    ByteEncoder* byte_encoder;          // UTF-8 byte encoding
    BPECache* bpe_cache;                // Performance cache

    // Configuration
    int vocab_size;                     // Usually 49408 for CLIP
    int max_token_length;              // Max token length
    bool add_prefix_space;             // CLIP behavior
    bool lowercase;                    // Usually true for CLIP

    // Special tokens (SD Compatible - FIXED!)
    int bos_token;                     // Beginning of sequence: 49406
    int eos_token;                     // End of sequence: 49407
    int pad_token;                     // ✅ Padding token: 0 (SD standard - FIXED!)
    int unk_token;                     // Unknown token: 49408

    // Performance stats
    uint64_t total_tokenizations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double avg_tokenization_time_ms;

    // Memory management
    NovaArena* arena;

} CLIPTokenizer;

// ============================================================================
// TOKEN SEQUENCE (Production-Ready)
// ============================================================================

typedef struct {
    int* tokens;        // Token sequence
    int length;         // Current length
    int capacity;       // Allocated capacity
    NovaArena* arena; // Memory ownership
} TokenSequence;

// Token sequence operations
TokenSequence* token_sequence_create(NovaArena* arena, int initial_capacity);
void token_sequence_append(TokenSequence* seq, int token);
void token_sequence_extend(TokenSequence* seq, const int* tokens, int count);
void token_sequence_free(TokenSequence* seq);

// ============================================================================
// BATCH PROCESSING (Production Optimization)
// ============================================================================

typedef struct {
    TokenSequence** sequences;  // Array of token sequences
    int batch_size;            // Number of sequences
    int max_length;           // Maximum sequence length
    bool pad_to_max;          // Whether to pad sequences
    NovaArena* arena;
} TokenBatch;

// Batch operations
TokenBatch* token_batch_create(NovaArena* arena, int batch_size, int max_length);
void token_batch_add_sequence(TokenBatch* batch, int index, TokenSequence* seq);
int* token_batch_get_padded_tokens(TokenBatch* batch, int* out_length);
void token_batch_free(TokenBatch* batch);

// ============================================================================
// CORE TOKENIZER API
// ============================================================================

// Lifecycle
CLIPTokenizer* clip_tokenizer_create(NovaArena* arena);
CLIPTokenizer* clip_tokenizer_load_from_files(NovaArena* arena,
                                             const char* vocab_file,
                                             const char* merges_file);
void clip_tokenizer_free(CLIPTokenizer* tokenizer);

// Tokenization
TokenSequence* clip_tokenize_text(CLIPTokenizer* tokenizer, const char* text);
TokenSequence* clip_tokenize_text_detailed(CLIPTokenizer* tokenizer, const char* text,
                                          bool add_special_tokens, int max_length);

// Encoding (tokenization + post-processing)
int clip_encode_text(CLIPTokenizer* tokenizer, const char* text,
                    int* tokens, int max_tokens, bool add_special_tokens);

// Decoding
char* clip_decode_tokens(CLIPTokenizer* tokenizer, const int* tokens, int num_tokens,
                        bool skip_special_tokens);

// Batch operations
TokenBatch* clip_tokenize_batch(CLIPTokenizer* tokenizer, const char** texts,
                               int batch_size, int max_length, bool pad);

// ============================================================================
// ADVANCED FEATURES (Production Requirements)
// ============================================================================

// Custom vocabulary loading
int clip_tokenizer_load_vocab(CLIPTokenizer* tokenizer, const char* vocab_json);
int clip_tokenizer_load_merges(CLIPTokenizer* tokenizer, const char* merges_txt);

// Performance tuning
void clip_tokenizer_set_cache_size(CLIPTokenizer* tokenizer, int cache_size);
void clip_tokenizer_enable_unicode_norm(CLIPTokenizer* tokenizer, bool enable);
void clip_tokenizer_set_regex_pattern(CLIPTokenizer* tokenizer, const char* pattern);

// Statistics and monitoring
typedef struct {
    uint64_t total_tokenizations;
    uint64_t cache_hit_ratio;
    double avg_tokenization_time_ms;
    double tokens_per_second;
    size_t memory_usage;
} TokenizerStats;

void clip_tokenizer_get_stats(CLIPTokenizer* tokenizer, TokenizerStats* stats);
void clip_tokenizer_reset_stats(CLIPTokenizer* tokenizer);

// ============================================================================
// SD-SPECIFIC CONSTANTS (CRITICAL FIX)
// ============================================================================

#define CLIP_VOCAB_SIZE 49408
#define CLIP_MAX_SEQ_LEN 77
#define CLIP_BOS_TOKEN 49406    // Beginning of sequence
#define CLIP_EOS_TOKEN 49407    // End of sequence
#define CLIP_PAD_TOKEN 0        // Padding token (SD standard - CRITICAL!)
#define CLIP_UNK_TOKEN 49408    // Unknown token

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    CLIP_SUCCESS = 0,
    CLIP_ERROR_MEMORY = -1,
    CLIP_ERROR_FILE_NOT_FOUND = -2,
    CLIP_ERROR_INVALID_VOCAB = -3,
    CLIP_ERROR_TOKEN_TOO_LONG = -4,
    CLIP_ERROR_UNICODE_ERROR = -5,
    CLIP_ERROR_REGEX_ERROR = -6
} CLIPError;

const char* clip_error_string(CLIPError error);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// String utilities
uint64_t clip_hash_string(const char* str);
char* clip_string_duplicate(const char* str, NovaArena* arena);
int clip_string_compare(const char* a, const char* b);

// Array utilities
int* clip_int_array_copy(const int* src, int len, NovaArena* arena);
int* clip_int_array_create(int len, int value, NovaArena* arena);

// File I/O helpers
char* clip_read_file(const char* filename, NovaArena* arena);
char** clip_split_lines(const char* content, int* num_lines, NovaArena* arena);

// ============================================================================
// TESTING & VALIDATION (Production Requirements)
// ============================================================================

// Test against OpenAI CLIP reference
typedef struct {
    const char* input_text;
    const int* expected_tokens;
    int expected_length;
} CLIPTestCase;

int clip_run_test_suite(CLIPTokenizer* tokenizer, CLIPTestCase* test_cases, int num_tests);

// Performance benchmarking
typedef struct {
    double tokens_per_second;
    double cache_hit_ratio;
    double avg_latency_ms;
    long peak_memory_usage;
} CLIPBenchmarkResult;

CLIPBenchmarkResult clip_benchmark_tokenizer(CLIPTokenizer* tokenizer,
                                           const char** test_texts,
                                           int num_texts,
                                           int iterations);

#endif // NOVA_CLIP_TOKENIZER_H
