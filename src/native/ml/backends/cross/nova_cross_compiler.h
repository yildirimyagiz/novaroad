/**
 * nova_cross_compiler.h - Multi-Target Cross-Compilation Support
 *
 * Supports: ARM64, x86_64, RISC-V, WASM, ARM32
 * Platform targets: Linux, macOS, Windows, iOS, Android, Bare Metal
 */
#ifndef NOVA_CROSS_COMPILER_H
#define NOVA_CROSS_COMPILER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// Target Architecture
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  NOVA_ARCH_X86_64 = 0,
  NOVA_ARCH_ARM64,
  NOVA_ARCH_ARM32,
  NOVA_ARCH_RISCV64,
  NOVA_ARCH_RISCV32,
  NOVA_ARCH_WASM32,
  NOVA_ARCH_WASM64,
  NOVA_ARCH_MIPS64,
  NOVA_ARCH_POWERPC64,
  NOVA_ARCH_COUNT
} NovaArch;

typedef enum {
  NOVA_OS_LINUX = 0,
  NOVA_OS_MACOS,
  NOVA_OS_WINDOWS,
  NOVA_OS_IOS,
  NOVA_OS_ANDROID,
  NOVA_OS_FREEBSD,
  NOVA_OS_BARE_METAL,
  NOVA_OS_WASI,
  NOVA_OS_COUNT
} NovaOS;

typedef enum {
  NOVA_ABI_SYSTEMV = 0,
  NOVA_ABI_WIN64,
  NOVA_ABI_AAPCS64,
  NOVA_ABI_EABI,
  NOVA_ABI_WASM,
  NOVA_ABI_COUNT
} NovaABI;

typedef enum {
  NOVA_OBJ_ELF = 0,
  NOVA_OBJ_MACHO,
  NOVA_OBJ_PE_COFF,
  NOVA_OBJ_WASM,
  NOVA_OBJ_COUNT
} NovaObjFormat;

typedef enum {
  NOVA_OPT_NONE = 0,   // -O0
  NOVA_OPT_SIZE,       // -Os
  NOVA_OPT_SPEED,      // -O2
  NOVA_OPT_AGGRESSIVE, // -O3
  NOVA_OPT_LTO         // LTO enabled
} NovaOptLevel;

// ═══════════════════════════════════════════════════════════════════════════
// Target Triple (arch-os-abi)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaArch arch;
  NovaOS os;
  NovaABI abi;
  NovaObjFormat obj_format;
  NovaOptLevel opt_level;

  // CPU features
  bool has_simd;
  bool has_fma;
  bool has_atomics;
  bool has_fp16;
  bool has_crypto;
  char cpu_name[64];  // e.g., "apple-m1", "skylake", "cortex-a76"
  char features[256]; // e.g., "+neon,+fp-armv8,+crypto"

  // Linker settings
  char sysroot[512];
  char linker_path[512];
  bool static_link;
  bool pic; // Position Independent Code
  bool pie; // Position Independent Executable

  // Output
  char output_name[256];
} NovaTarget;

// ═══════════════════════════════════════════════════════════════════════════
// Cross-Compilation Config
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaTarget target;
  const char *source_files[256];
  int source_count;
  const char *include_paths[64];
  int include_count;
  const char *lib_paths[64];
  int lib_count;
  const char *link_libs[64];
  int link_lib_count;
  const char *defines[128];
  int define_count;
  bool debug_info;
  bool strip_symbols;
  bool emit_llvm_ir;
  bool emit_assembly;
} NovaCrossConfig;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

/** Detect host architecture */
NovaArch nova_detect_host_arch(void);

/** Detect host OS */
NovaOS nova_detect_host_os(void);

/** Get LLVM triple string: e.g., "aarch64-apple-darwin" */
const char *nova_target_triple(const NovaTarget *target);

/** Create default target for current host */
NovaTarget nova_target_host(void);

/** Create target from triple string */
NovaTarget nova_target_from_triple(const char *triple);

/** Create pre-configured targets */
NovaTarget nova_target_linux_x86_64(void);
NovaTarget nova_target_linux_arm64(void);
NovaTarget nova_target_macos_arm64(void);
NovaTarget nova_target_macos_x86_64(void);
NovaTarget nova_target_windows_x86_64(void);
NovaTarget nova_target_ios_arm64(void);
NovaTarget nova_target_android_arm64(void);
NovaTarget nova_target_riscv64(void);
NovaTarget nova_target_wasm32(void);

/** Cross-compile sources for target */
int nova_cross_compile(const NovaCrossConfig *config);

/** Link object files for target */
int nova_cross_link(const NovaTarget *target, const char **obj_files,
                      int obj_count, const char *output);

/** Check if cross toolchain is available for target */
bool nova_cross_toolchain_available(const NovaTarget *target);

/** Get arch name string */
const char *nova_arch_name(NovaArch arch);

/** Get OS name string */
const char *nova_os_name(NovaOS os);

/** Print target summary */
void nova_target_print(const NovaTarget *target);

/** Print all available targets */
void nova_print_available_targets(void);

#endif // NOVA_CROSS_COMPILER_H
