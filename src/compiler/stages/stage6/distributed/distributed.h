#ifndef NOVA_DISTRIBUTED_H
#define NOVA_DISTRIBUTED_H

#include "p2p_network.h"

// Main distributed compilation orchestration
void nova_distributed_orchestrate(const char *target);

// Task distribution
void distribute_tasks(P2PNetwork *network, const char *ir_data, size_t data_size);

// Result collection
void collect_results(P2PNetwork *network, void *result_buffer, size_t buffer_size);

// Load balancing
void balance_load(P2PNetwork *network);

#endif
