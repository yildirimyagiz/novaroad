/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ERRORS — Birleşik Hata Kodları & Raporlama
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_ERRORS_H
#define NOVA_ERRORS_H

#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * ERROR CODES
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  ZEN_OK = 0,

  /* Memory errors (100-199) */
  ZEN_ERR_ALLOC_FAILED = 100,
  ZEN_ERR_NULL_POINTER = 101,
  ZEN_ERR_BUFFER_OVERFLOW = 102,
  ZEN_ERR_USE_AFTER_FREE = 103,
  ZEN_ERR_DOUBLE_FREE = 104,

  /* Shape / dimension errors (200-299) */
  ZEN_ERR_SHAPE_MISMATCH = 200,
  ZEN_ERR_DIM_OUT_OF_RANGE = 201,
  ZEN_ERR_INVALID_STRIDE = 202,
  ZEN_ERR_EMPTY_TENSOR = 203,

  /* Numeric errors (300-399) */
  ZEN_ERR_NAN_DETECTED = 300,
  ZEN_ERR_INF_DETECTED = 301,
  ZEN_ERR_NUMERIC_OVERFLOW = 302,
  ZEN_ERR_DIVISION_BY_ZERO = 303,

  /* Type errors (400-499) */
  ZEN_ERR_TYPE_MISMATCH = 400,
  ZEN_ERR_INVALID_CAST = 401,
  ZEN_ERR_UNSUPPORTED_TYPE = 402,

  /* Proof / Obligation errors (500-599) */
  ZEN_ERR_PROOF_FAILED = 500,
  ZEN_ERR_PROOF_TIMEOUT = 501,
  ZEN_ERR_OBLIGATION_REFUTED = 502,
  ZEN_ERR_OBLIGATION_PENDING = 503,
  ZEN_ERR_SOLVER_ERROR = 504,
  ZEN_ERR_SMT2_EMIT_FAILED = 505,
  ZEN_ERR_PROOF_CACHE_MISS = 506,

  /* Policy errors (600-699) */
  ZEN_ERR_EXECUTION_DENIED = 600,
  ZEN_ERR_POLICY_VIOLATION = 601,
  ZEN_ERR_UNTRUSTED_KERNEL = 602,
  ZEN_ERR_AUDIT_WRITE_FAILED = 603,

  /* Symbolic IR errors (700-799) */
  ZEN_ERR_SYM_POOL_EXHAUSTED = 700,
  ZEN_ERR_SYM_INVALID_EXPR = 701,
  ZEN_ERR_SYM_TYPE_ERROR = 702,
  ZEN_ERR_SYM_UNBOUND_VAR = 703,

  /* I/O errors (800-899) */
  ZEN_ERR_FILE_OPEN = 800,
  ZEN_ERR_FILE_WRITE = 801,
  ZEN_ERR_FILE_READ = 802,

  /* Kernel / Compute errors (900-999) */
  ZEN_ERR_KERNEL_NOT_FOUND = 900,
  ZEN_ERR_KERNEL_INVALID = 901,
  ZEN_ERR_JIT_COMPILE_FAILED = 902,
  ZEN_ERR_GRAPH_CYCLE = 903,
  ZEN_ERR_OPTIMIZATION_INVALID = 904,

  /* Attestation errors (1000-1099) */
  ZEN_ERR_ATTEST_HASH_MISMATCH = 1000,
  ZEN_ERR_ATTEST_SIGNATURE_BAD = 1001,
  ZEN_ERR_ATTEST_EXPIRED = 1002,
} NovaError;

/* ═══════════════════════════════════════════════════════════════════════════
 * ERROR REPORTING
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Report an error with context
 */
void nova_report_error(const char *msg, int code);

/**
 * Get human-readable error name
 */
const char *nova_error_name(NovaError err);

/**
 * Get error description
 */
const char *nova_error_description(NovaError err);

/**
 * Set global error handler callback
 */
typedef void (*NovaErrorHandler)(NovaError err, const char *file, int line,
                                   const char *msg);
void nova_set_error_handler(NovaErrorHandler handler);

/**
 * Report error with file/line context
 */
#define NOVA_ERROR(err, msg)                                                 \
  nova_report_error_ctx((err), __FILE__, __LINE__, (msg))

void nova_report_error_ctx(NovaError err, const char *file, int line,
                             const char *msg);

#endif /* NOVA_ERRORS_H */
