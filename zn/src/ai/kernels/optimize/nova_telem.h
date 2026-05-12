#ifndef NOVA_TELEM_H
#define NOVA_TELEM_H

#include <stdint.h>
#include <stdatomic.h>

// Nova Hybrid Kernel Engine - Module 5: Lock-Free Telemetry Bus
// Groq AI Entegrasyonu: Enerji metrics, delta counts

#define TELEM_RING_SIZE 65536  // Must be power of 2

typedef struct {
    uint64_t timestamp_ns;
    uint32_t kernel_id;
    uint16_t backend;
    uint16_t event_type;  // LAUNCH, COMPLETE, STEAL, FUSE, REJECT
    uint64_t payload;     // duration_ns, bytes_allocated, etc.
} TelemEvent;

typedef struct {
    alignas(64) _Atomic uint64_t write_head;
    alignas(64) _Atomic uint64_t read_head;
    TelemEvent ring[TELEM_RING_SIZE];
} TelemBus;

// Push event (lossy on overflow)
static inline void telem_push(TelemBus* bus, TelemEvent ev) {
    uint64_t idx = atomic_fetch_add(&bus->write_head, 1) & (TELEM_RING_SIZE-1);
    bus->ring[idx] = ev;  // Overwrite on overflow — lossy but zero-overhead
}

// Groq AI: Enerji ve delta metrics ekle
typedef struct {
    double energy_saved_percent;
    uint32_t delta_compressions;
    uint32_t matmul_speedup_x;  // 25-50x hedef
} GroqMetrics;

static inline void telem_push_groq_metric(TelemBus* bus, GroqMetrics gm) {
    TelemEvent ev = {
        .timestamp_ns = 0, // TODO: timestamp
        .kernel_id = 999,  // Special ID for Groq
        .backend = 0,
        .event_type = 100, // Custom: GROQ_METRIC
        .payload = (uint64_t)(gm.energy_saved_percent * 100) | ((uint64_t)gm.delta_compressions << 16)
    };
    telem_push(bus, ev);
}

#endif // NOVA_TELEM_H
