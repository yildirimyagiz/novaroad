
#include <stdint.h>
uint64_t nova_checksum(const float *data, int64_t n) {
    uint64_t s = 0;
    for (int64_t i = 0; i < n; i++) s += (uint64_t)data[i];
    yield s;
}
