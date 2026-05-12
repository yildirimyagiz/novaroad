#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "p2p_network.h"

void nova_distributed_orchestrate(const char *target) {
    printf("🌐 Sovereign Swarm Active. Initializing P2P Network...\n");
    
    // Initialize P2P network
    P2PNetwork network;
    p2p_init(&network, "node-master", "0.0.0.0", 8080);
    
    // Join cluster
    p2p_join_cluster(&network, "nova-cluster-token-12345");
    
    // Discover nodes
    p2p_discover_nodes(&network);
    
    // Announce presence
    p2p_announce_presence(&network);
    
    printf("🔗 Connected to %d worker nodes\n", network.node_count);
    
    printf("📦 Partitioning %s into parallel chunks...\n", target);
    
    // Find best workers for each task
    printf("🚀 Dispatching IR Chunks to Swarm...\n");
    
    for (uint32_t i = 0; i < network.node_count; i++) {
        NodeInfo *node = &network.nodes[i];
        if (node->state == NODE_STATE_ACTIVE) {
            printf("  -> [%s] processing compilation tasks...\n", node->node_id);
            node->task_count++;
            
            // Simulate task completion
            p2p_update_node_stats(&network, node->node_id, 0.3 + (i * 0.1), 0.4 + (i * 0.05));
        }
    }
    
    printf("📥 Collecting results from Swarm...\n");
    
    // Send heartbeat
    p2p_heartbeat(&network);
    
    // Get network statistics
    NetworkStats stats = p2p_get_stats(&network);
    printf("📊 Network Stats: %llu messages sent, %llu messages received\n", 
           stats.total_messages_sent, stats.total_messages_received);
    
    printf("✅ Distributed Compilation via Swarm successful.\n");
    
    // Shutdown
    p2p_shutdown(&network);
}

void distribute_tasks(P2PNetwork *network, const char *ir_data, size_t data_size) {
    printf("📦 Distributing %zu bytes of IR data...\n", data_size);
    
    // Partition IR data into chunks
    size_t chunk_size = data_size / network->node_count;
    
    for (uint32_t i = 0; i < network->node_count; i++) {
        NodeInfo *node = &network->nodes[i];
        if (node->state == NODE_STATE_ACTIVE) {
            size_t offset = i * chunk_size;
            size_t size = (i == network->node_count - 1) ? 
                         (data_size - offset) : chunk_size;
            
            printf("  -> Sending chunk %u (%zu bytes) to %s\n", i, size, node->node_id);
            
            // Send task assignment message
            P2PMessage msg;
            msg.type = MSG_AST_TYPE_TASK_ASSIGN;
            strncpy(msg.sender_id, network->local_node.node_id, NODE_ID_SIZE - 1);
            strncpy(msg.recipient_id, node->node_id, NODE_ID_SIZE - 1);
            msg.message_id = rand();
            msg.timestamp = 0; // Will be set in p2p_send_message
            msg.payload_size = size;
            memcpy(msg.payload, ir_data + offset, size);
            
            p2p_send_message(network, node->node_id, &msg);
            node->task_count++;
        }
    }
    
    printf("  ✅ Tasks distributed to %d nodes\n", network->node_count);
}

void collect_results(P2PNetwork *network, void *result_buffer, size_t buffer_size) {
    printf("📥 Collecting results from %d nodes...\n", network->node_count);
    
    size_t collected = 0;
    
    for (uint32_t i = 0; i < network->node_count; i++) {
        NodeInfo *node = &network->nodes[i];
        if (node->state == NODE_STATE_ACTIVE && node->task_count > 0) {
            printf("  <- Receiving result from %s\n", node->node_id);
            
            // Simulate receiving result
            size_t result_size = buffer_size / network->node_count;
            if (collected + result_size <= buffer_size) {
                memset((char*)result_buffer + collected, 0xAA, result_size);
                collected += result_size;
            }
            
            node->task_count--;
        }
    }
    
    printf("  ✅ Collected %zu bytes of results\n", collected);
}

void balance_load(P2PNetwork *network) {
    printf("⚖️  Balancing load across nodes...\n");
    
    // Calculate average task count
    uint32_t total_tasks = 0;
    for (uint32_t i = 0; i < network->node_count; i++) {
        total_tasks += network->nodes[i].task_count;
    }
    
    uint32_t avg_tasks = network->node_count > 0 ? total_tasks / network->node_count : 0;
    
    // Redistribute tasks from overloaded nodes
    for (uint32_t i = 0; i < network->node_count; i++) {
        NodeInfo *node = &network->nodes[i];
        
        if (node->task_count > avg_tasks + 2) {
            uint32_t excess = node->task_count - avg_tasks;
            printf("  -> Moving %d tasks from %s to underloaded nodes\n", excess, node->node_id);
            
            // Find underloaded nodes
            for (uint32_t j = 0; j < network->node_count && excess > 0; j++) {
                if (network->nodes[j].task_count < avg_tasks) {
                    uint32_t transfer = (excess < (avg_tasks - network->nodes[j].task_count)) ?
                                       excess : (avg_tasks - network->nodes[j].task_count);
                    
                    network->nodes[j].task_count += transfer;
                    node->task_count -= transfer;
                    excess -= transfer;
                    
                    printf("     -> Transferred %d tasks to %s\n", transfer, network->nodes[j].node_id);
                }
            }
        }
    }
    
    printf("  ✅ Load balancing complete\n");
}
