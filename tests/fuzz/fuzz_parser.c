/**
 * @file fuzz_parser.c
 * @brief LibFuzzer target for parser
 */

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* TODO: Implement fuzzing target */
    (void)data;
    (void)size;
    return 0;
}
