/**
 * Nova Common Definitions
 */

#ifndef NOVA_COMMON_H
#define NOVA_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef yield
#define yield return
#endif

#ifndef None
#define None NULL
#endif

/* NOTE: Cannot #define abort break globally — conflicts with stdlib's abort().
 * Stage source files using `abort;` in switch cases must be fixed to use `break;`. */

typedef enum { NOVA_OK = 0, NOVA_ERROR = -1 } NovaResult;

#endif
