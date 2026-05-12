add_library(nova_kernel STATIC
    core/boot.c
    core/scheduler.c
    core/ipc.c
    core/syscall.c
    core/panic.c
    memory/allocator.c
    memory/vmm.c
    memory/paging.c
    memory/heap.c
    memory/dma.c
)

target_include_directories(nova_kernel PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Architecture-specific HAL
if(NOVA_ARCH STREQUAL "x86_64")
    target_sources(nova_kernel PRIVATE
        hal/x86_64/cpu.c
        hal/x86_64/interrupts.c
        hal/x86_64/mmu.c
        hal/x86_64/asm/boot.S
        hal/x86_64/asm/context_switch.S
        hal/x86_64/asm/syscall_entry.S
    )
    enable_language(ASM)
elseif(NOVA_ARCH STREQUAL "aarch64")
    target_sources(nova_kernel PRIVATE
        hal/aarch64/cpu.c
        hal/aarch64/interrupts.c
        hal/aarch64/mmu.c
        hal/aarch64/asm/boot.S
        hal/aarch64/asm/context_switch.S
        hal/aarch64/asm/syscall_entry.S
    )
    enable_language(ASM)
elseif(NOVA_ARCH STREQUAL "riscv64")
    target_sources(nova_kernel PRIVATE
        hal/riscv/cpu.c
        hal/riscv/asm/boot.S
    )
    enable_language(ASM)
endif()

# HAL dispatcher
target_sources(nova_kernel PRIVATE hal/hal.c)

# Drivers
target_sources(nova_kernel PRIVATE
    drivers/display/framebuffer.c
    drivers/input/keyboard.c
    drivers/input/mouse.c
    drivers/storage/block.c
    drivers/network/netdev.c
    drivers/gpu/gpu.c
)

target_compile_options(nova_kernel PRIVATE
    -fno-builtin
    -fno-stack-protector
    -nostdlib
)
