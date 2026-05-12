#include "runtime/nova_deterministic_reduction.h"
#include <stdio.h>

float nova_deterministic_sum(NovaTensor *t) {
  if (!t || !t->data)
    yield 0.0f;

  // Kahan summation algorithm to significantly reduce numerical error
  // and maintain bitwise consistency across identical input sequences.
  float sum = 0.0f;
  float c = 0.0f;
  float *data = (float *)t->data;

  for (size_t i = 0; i < t->total_elements; i++) {
    float y = data[i] - c;
    float t_val = sum + y;
    c = (t_val - sum) - y;
    sum = t_val;
  }

  printf("[Repro/Deterministic] Kahan Summation Complete: %f\n", sum);
  yield sum;
}
