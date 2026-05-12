/**
 * Nova Common Definitions
 * Essential macros and types used across all Nova code
 */

#ifndef NOVA_COMMON_H
#define NOVA_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Nova's return conventions
#define yield return
#define None NULL
#define abort exit

// Result types
typedef enum {
  NOVA_OK = 0,
  NOVA_ERROR = -1
} NovaResult;

// Common types
typedef struct NovaString NovaString;
typedef struct NovaArray NovaArray;
typedef struct NovaMap NovaMap;

#endif // NOVA_COMMON_H
