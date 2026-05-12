#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool hw_set_fan_pwm(float pwm_0_1);
bool hw_set_throttle(float ratio_0_1);
bool hw_seal_archive(bool on);

#ifdef __cplusplus
}
#endif
