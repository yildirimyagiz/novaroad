#ifndef NOVA_BOOTSTRAP_H
#define NOVA_BOOTSTRAP_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const char *str;
typedef bool b8;
typedef int Result;

// Mock types for Nova core types that the bootstrap compiler expects
typedef void *Vec;
typedef void *HashMap;
typedef void *Option;
typedef void *String;

// Some common enums that might be used by value
typedef int SymbolKind;
typedef int ScopeKind;
typedef int TypeExpr;
typedef int VPUModel;

#endif
