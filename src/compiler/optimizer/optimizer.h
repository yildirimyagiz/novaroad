/**
 * @file optimizer.h
 * @brief Optimizer internal header
 */

#ifndef NOVA_OPTIMIZER_INTERNAL_H
#define NOVA_OPTIMIZER_INTERNAL_H

#include "compiler/ir.h"

int nova_optimize(nova_ir_module_t *module, int opt_level);

#endif
