#ifndef NOVA_FEDERATED_H
#define NOVA_FEDERATED_H

#include "federated_learning.h"

// Main federated learning bridge
void nova_federated_update(int optimization_hits);

// Model synchronization
void federated_sync_model(FederatedLearning *fl);

// Pattern extraction
void extract_optimization_patterns(const char *ir_data, float *features, uint32_t count);

#endif
