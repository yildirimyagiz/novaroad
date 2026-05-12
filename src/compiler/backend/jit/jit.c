/**
 * @file jit.c
 * @brief JIT compiler implementation
 */

#include "jit.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "../../../../include/std/alloc.h"


#define JIT_CODE_SIZE (64 * 1024)  /* 64KB executable memory */

struct jit_buffer {
    uint8_t *code;
    size_t size;
    size_t capacity;
    void *exec_mem;
};

struct nova_jit_compiler {
    jit_buffer_t buffer;
    const char *arch;
};

/* Architecture detection */
static const char *detect_arch(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#else
    return "unknown";
#endif
}

/* Allocate executable memory */
static void *alloc_exec_memory(size_t size)
{
    void *mem = mmap(NULL, size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (mem == MAP_FAILED) ? NULL : mem;
}

/* Create JIT compiler */
nova_jit_compiler_t *nova_jit_create(void)
{
    nova_jit_compiler_t *jit = (nova_jit_compiler_t *)nova_alloc(sizeof(nova_jit_compiler_t));
    if (!jit) return NULL;
    
    jit->arch = detect_arch();
    jit->buffer.capacity = JIT_CODE_SIZE;
    jit->buffer.size = 0;
    jit->buffer.exec_mem = alloc_exec_memory(JIT_CODE_SIZE);
    jit->buffer.code = (uint8_t *)jit->buffer.exec_mem;
    
    if (!jit->buffer.exec_mem) {
        nova_free(jit);
        return NULL;
    }
    
    return jit;
}

/* Emit byte to JIT buffer */
static void jit_emit_byte(jit_buffer_t *buf, uint8_t byte)
{
    if (buf->size < buf->capacity) {
        buf->code[buf->size++] = byte;
    }
}

/* Emit bytes to JIT buffer */
static void jit_emit_bytes(jit_buffer_t *buf, const uint8_t *bytes, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        jit_emit_byte(buf, bytes[i]);
    }
}

/* Forward declarations for arch-specific compilers */
extern int nova_jit_compile_x86_64(nova_jit_compiler_t *jit, IRModule *ir);
extern int nova_jit_compile_aarch64(nova_jit_compiler_t *jit, IRModule *ir);

/* Compile IR to native code */
int nova_jit_compile(nova_jit_compiler_t *jit, IRModule *ir)
{
    if (!jit || !ir) return -1;
    
    /* Dispatch to architecture-specific compiler */
    if (strcmp(jit->arch, "x86_64") == 0) {
        return nova_jit_compile_x86_64(jit, ir);
    } else if (strcmp(jit->arch, "aarch64") == 0) {
        return nova_jit_compile_aarch64(jit, ir);
    }
    
    return -1;  /* Unsupported architecture */
}

/* Execute JIT-compiled code */
void *nova_jit_get_function(nova_jit_compiler_t *jit)
{
    if (!jit || !jit->buffer.exec_mem) return NULL;
    return jit->buffer.exec_mem;
}

/* Destroy JIT compiler */
void nova_jit_destroy(nova_jit_compiler_t *jit)
{
    if (!jit) return;
    
    if (jit->buffer.exec_mem) {
        munmap(jit->buffer.exec_mem, jit->buffer.capacity);
    }
    
    nova_free(jit);
}

/* Get buffer for emitting code */
jit_buffer_t *nova_jit_get_buffer(nova_jit_compiler_t *jit)
{
    return &jit->buffer;
}

/* Emit helper functions */
void nova_jit_emit_byte(nova_jit_compiler_t *jit, uint8_t byte)
{
    jit_emit_byte(&jit->buffer, byte);
}

void nova_jit_emit_bytes(nova_jit_compiler_t *jit, const uint8_t *bytes, size_t len)
{
    jit_emit_bytes(&jit->buffer, bytes, len);
}
