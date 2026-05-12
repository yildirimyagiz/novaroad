#ifndef NOVA_RL_H
#define NOVA_RL_H

#include <stdint.h>
#include <stdlib.h>

/* ============================================================================
 * Replay Buffer - Experience replay for RL agents
 * ============================================================================ */

typedef struct {
    float *states;          /* [capacity * state_dim] */
    float *actions;         /* [capacity * action_dim] */
    float *rewards;         /* [capacity] */
    float *next_states;     /* [capacity * state_dim] */
    uint8_t *dones;         /* [capacity] - terminal flags */
    
    size_t capacity;
    size_t size;            /* current number of samples */
    size_t head;            /* insertion pointer */
    size_t state_dim;
    size_t action_dim;
} NovaReplayBuffer;

NovaReplayBuffer* nova_replay_buffer_create(size_t capacity, size_t state_dim, size_t action_dim);
void nova_replay_buffer_push(NovaReplayBuffer *buf, const float *state, const float *action, 
                             float reward, const float *next_state, uint8_t done);
void nova_replay_buffer_sample(NovaReplayBuffer *buf, size_t batch_size,
                               float *out_states, float *out_actions, float *out_rewards,
                               float *out_next_states, uint8_t *out_dones);
void nova_replay_buffer_free(NovaReplayBuffer *buf);

/* ============================================================================
 * MLP - Multi-Layer Perceptron for actor/critic networks
 * ============================================================================ */

typedef struct {
    float **weights;        /* weights[layer_idx] -> [out_size * in_size] */
    float **biases;         /* biases[layer_idx] -> [out_size] */
    size_t *layer_sizes;    /* layer_sizes[layer_idx] = size of layer */
    size_t n_layers;
} NovaMLP;

NovaMLP* nova_mlp_create(const size_t *layer_sizes, size_t n_layers);
void nova_mlp_forward(NovaMLP *mlp, const float *input, float *output);
void nova_mlp_free(NovaMLP *mlp);

/* ============================================================================
 * SAC (Soft Actor-Critic) Agent
 * ============================================================================ */

typedef struct {
    NovaMLP *actor;           /* policy network */
    NovaMLP *critic1;         /* Q-function 1 */
    NovaMLP *critic2;         /* Q-function 2 */
    NovaMLP *target_critic1;  /* target Q-function 1 */
    NovaMLP *target_critic2;  /* target Q-function 2 */
    
    float alpha;              /* entropy coefficient */
    float log_alpha;          /* log of alpha */
    float target_entropy;     /* target entropy for auto-tuning */
    
    size_t state_dim;
    size_t action_dim;
    float gamma;              /* discount factor */
    float tau;                /* polyak averaging coefficient */
} NovaSACAgent;

NovaSACAgent* nova_sac_create(size_t state_dim, size_t action_dim, size_t hidden_dim,
                              float gamma, float tau, float alpha);
void nova_sac_select_action(NovaSACAgent *agent, const float *state, float *action, uint8_t explore);
void nova_sac_update(NovaSACAgent *agent, NovaReplayBuffer *buffer, size_t batch_size,
                     float lr_actor, float lr_critic, float lr_alpha);
void nova_sac_soft_update(NovaMLP *target, NovaMLP *source, float tau);
void nova_sac_free(NovaSACAgent *agent);

/* ============================================================================
 * TD3 (Twin Delayed DDPG) Agent
 * ============================================================================ */

typedef struct {
    NovaMLP *actor;           /* deterministic policy */
    NovaMLP *critic1;         /* Q-function 1 */
    NovaMLP *critic2;         /* Q-function 2 */
    NovaMLP *target_actor;    /* target policy */
    NovaMLP *target_critic1;  /* target Q-function 1 */
    NovaMLP *target_critic2;  /* target Q-function 2 */
    
    float action_noise;       /* std of exploration noise */
    float policy_noise;       /* std of target smoothing noise */
    size_t policy_delay;      /* update actor every N critic updates */
    size_t update_count;      /* counter for delayed updates */
    
    size_t state_dim;
    size_t action_dim;
    float gamma;              /* discount factor */
    float tau;                /* polyak averaging coefficient */
    float action_noise_clip;  /* clipping for target noise */
} NovaTD3Agent;

NovaTD3Agent* nova_td3_create(size_t state_dim, size_t action_dim, size_t hidden_dim,
                              float gamma, float tau, float action_noise, size_t policy_delay);
void nova_td3_select_action(NovaTD3Agent *agent, const float *state, float *action, uint8_t explore);
void nova_td3_update(NovaTD3Agent *agent, NovaReplayBuffer *buffer, size_t batch_size, float lr);
void nova_td3_free(NovaTD3Agent *agent);

#endif
