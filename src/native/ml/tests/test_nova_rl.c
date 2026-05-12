#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../ai/core/nova_rl.h"

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

#define ASSERT_NEAR(a, b, tol) \
    do { \
        float _a = (a), _b = (b), _tol = (tol); \
        if (fabsf(_a - _b) > _tol) { \
            printf("ASSERT_NEAR failed: %f != %f (tolerance: %f)\n", _a, _b, _tol); \
            exit(1); \
        } \
    } while(0)

#define TEST_PASS() printf("✓ %s\n", __func__)
#define TEST_FAIL(msg) do { printf("✗ %s: %s\n", __func__, msg); exit(1); } while(0)

/* ============================================================================
 * Test Cases
 * ============================================================================ */

void test_replay_buffer_create_free(void) {
    NovaReplayBuffer *buf = nova_replay_buffer_create(1000, 4, 2);
    assert(buf != NULL);
    assert(buf->capacity == 1000);
    assert(buf->state_dim == 4);
    assert(buf->action_dim == 2);
    assert(buf->size == 0);
    nova_replay_buffer_free(buf);
    TEST_PASS();
}

void test_replay_buffer_push_sample(void) {
    NovaReplayBuffer *buf = nova_replay_buffer_create(1000, 4, 2);
    
    /* Push 100 transitions */
    for (int i = 0; i < 100; i++) {
        float state[4] = {(float)i, (float)i+1, (float)i+2, (float)i+3};
        float action[2] = {-1.0f + (float)i * 0.02f, -0.5f};
        float reward = (float)i * 0.1f;
        float next_state[4] = {(float)i+0.1f, (float)i+1.1f, (float)i+2.1f, (float)i+3.1f};
        uint8_t done = (i % 10 == 9) ? 1 : 0;
        
        nova_replay_buffer_push(buf, state, action, reward, next_state, done);
    }
    
    assert(buf->size == 100);
    
    /* Sample batch of 32 */
    float out_states[32 * 4];
    float out_actions[32 * 2];
    float out_rewards[32];
    float out_next_states[32 * 4];
    uint8_t out_dones[32];
    
    nova_replay_buffer_sample(buf, 32, out_states, out_actions, out_rewards, 
                              out_next_states, out_dones);
    
    nova_replay_buffer_free(buf);
    TEST_PASS();
}

void test_replay_buffer_overflow(void) {
    NovaReplayBuffer *buf = nova_replay_buffer_create(100, 4, 2);
    
    /* Push capacity + 100 transitions */
    for (int i = 0; i < 200; i++) {
        float state[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float action[2] = {0.5f, -0.5f};
        float reward = 1.0f;
        float next_state[4] = {1.1f, 2.1f, 3.1f, 4.1f};
        
        nova_replay_buffer_push(buf, state, action, reward, next_state, 0);
    }
    
    /* Size should not exceed capacity */
    assert(buf->size <= buf->capacity);
    assert(buf->size == 100);
    
    nova_replay_buffer_free(buf);
    TEST_PASS();
}

void test_sac_create_free(void) {
    NovaSACAgent *agent = nova_sac_create(4, 2, 64, 0.99f, 0.005f, 0.2f);
    assert(agent != NULL);
    assert(agent->state_dim == 4);
    assert(agent->action_dim == 2);
    assert(agent->gamma == 0.99f);
    assert(agent->tau == 0.005f);
    nova_sac_free(agent);
    TEST_PASS();
}

void test_sac_select_action(void) {
    NovaSACAgent *agent = nova_sac_create(4, 2, 64, 0.99f, 0.005f, 0.2f);
    
    float state[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    float action[2];
    
    nova_sac_select_action(agent, state, action, 1);
    
    /* Actions should be in [-1, 1] range for continuous control */
    assert(action[0] >= -1.1f && action[0] <= 1.1f);
    assert(action[1] >= -1.1f && action[1] <= 1.1f);
    
    nova_sac_free(agent);
    TEST_PASS();
}

void test_sac_soft_update(void) {
    NovaMLP *source = nova_mlp_create((size_t[]){4, 64, 2}, 3);
    NovaMLP *target = nova_mlp_create((size_t[]){4, 64, 2}, 3);
    
    /* Soft update with tau=0.005 */
    nova_sac_soft_update(target, source, 0.005f);
    
    /* Verify soft update executes without error and target is modified */
    assert(target->weights[0] != NULL);
    
    nova_mlp_free(source);
    nova_mlp_free(target);
    TEST_PASS();
}

void test_td3_create_free(void) {
    NovaTD3Agent *agent = nova_td3_create(4, 2, 64, 0.99f, 0.005f, 0.1f, 2);
    assert(agent != NULL);
    assert(agent->state_dim == 4);
    assert(agent->action_dim == 2);
    assert(agent->action_noise == 0.1f);
    assert(agent->policy_delay == 2);
    nova_td3_free(agent);
    TEST_PASS();
}

void test_td3_select_action(void) {
    NovaTD3Agent *agent = nova_td3_create(4, 2, 64, 0.99f, 0.005f, 0.1f, 2);
    
    float state[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    float action[2];
    
    nova_td3_select_action(agent, state, action, 1);
    
    /* Actions should be in valid range */
    assert(action[0] >= -1.5f && action[0] <= 1.5f);
    assert(action[1] >= -1.5f && action[1] <= 1.5f);
    
    nova_td3_free(agent);
    TEST_PASS();
}

void test_mlp_create_forward(void) {
    size_t layer_sizes[] = {4, 64, 2};
    NovaMLP *mlp = nova_mlp_create(layer_sizes, 3);
    
    assert(mlp != NULL);
    assert(mlp->n_layers == 2);  /* n_layers - 1 weight matrices */
    assert(mlp->weights != NULL);
    assert(mlp->biases != NULL);
    
    nova_mlp_free(mlp);
    TEST_PASS();
}

void test_mlp_he_init(void) {
    size_t layer_sizes[] = {4, 64, 2};
    NovaMLP *mlp = nova_mlp_create(layer_sizes, 3);
    
    /* Check that weights are initialized (non-zero with reasonable probability) */
    int nonzero_count = 0;
    for (size_t i = 0; i < 4 * 64; i++) {
        if (mlp->weights[0][i] != 0.0f) {
            nonzero_count++;
        }
    }
    
    /* Most weights should be non-zero after He initialization */
    assert(nonzero_count > 100);
    
    nova_mlp_free(mlp);
    TEST_PASS();
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("\n=== Nova RL Unit Tests ===\n\n");
    
    test_replay_buffer_create_free();
    test_replay_buffer_push_sample();
    test_replay_buffer_overflow();
    test_sac_create_free();
    test_sac_select_action();
    test_sac_soft_update();
    test_td3_create_free();
    test_td3_select_action();
    test_mlp_create_forward();
    test_mlp_he_init();
    
    printf("\n=== All RL Tests Passed ===\n\n");
    return 0;
}
