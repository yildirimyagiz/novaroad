/**
 * @file nova.h
 * @brief Main umbrella header for Nova programming language
 */

#ifndef NOVA_H
#define NOVA_H

#include "config/config.h"
#include "config/version.h"

/* Kernel subsystem */
#include "kernel/hal.h"
#include "kernel/ipc.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/sched.h"
#include "kernel/syscall.h"

/* Runtime subsystem */
#include "plugin/plugin.h"
#include "runtime/actor.h"
#include "runtime/async.h"
#include "runtime/gc.h"
#include "runtime/runtime.h"
#include "runtime/thread.h"
#include "runtime/value.h"

/* Compiler subsystem */
#include "compiler/ast.h"
#include "compiler/codegen.h"
#include "compiler/compiler.h"
#include "compiler/ir.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/types.h"

/* AI subsystem */
#include "ai/autograd.h"
#include "ai/inference.h"
#include "ai/nn.h"
#include "ai/tensor.h"

/* Security subsystem */
#include "security/capabilities.h"
#include "security/crypto.h"
#include "security/sandbox.h"

/* Platform abstraction */
#include "platform/atomic.h"
#include "platform/fs.h"
#include "platform/hal.h"
#include "platform/net.h"
#include "platform/platform.h"
#include "platform/simd.h"
#include "platform/thread.h"
#include "platform/wasm.h"

/* Standard library */
#include "std/alloc.h"
#include "std/collections.h"
#include "std/io.h"
#include "std/net.h"
#include "std/string.h"

#endif /* NOVA_H */
