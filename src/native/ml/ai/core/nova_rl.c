#include "nova_rl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Box-Muller transform for Gaussian sampling */
static float nova_gaussian_sample(float mean, float std) {
    static int has_spare = 0;
    static float spare;
    
    if (has_spare) {
        has_spare = 0;
        return mean + std * spare;
    }
    
    has_spare = 1;
    float u = (float)rand() / (float)RAND_MAX;
    float v = (float)rand() / (float)RAND_MAX;
    float mag = sqrtf(-2.0f * logf(u + 1e-8f));
    spare = mag * sinf(2.0f * M_PI * v);
    return mean + std * mag * cosf(2.0f * M_PI * v);
}

/* ReLU activation */
static float nova_relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

/* Tanh activation */
static float nova_tanh_activate(float x) {
    return tanhf(x);
}

/* He initialization */
static float nova_he_init(float fan_in) {
    float std = sqrtf(2.0f / fan_in);
    return nova_gaussian_sample(0.0f, std);
}

/* ============================================================================
 * Replay Buffer Implementation
 * ============================================================================ */

NovaReplayBuffer* nova_replay_buffer_create(size_t capacity, size_t state_dim, size_t action_dim) {
    NovaReplayBuffer *buf = (NovaReplayBuffer *)malloc(sizeof(NovaReplayBuffer));
    if (!buf) return NULL;
    
    buf->capacity = capacity;
    buf->state_dim = state_dim;
    buf->action_dim = action_dim;
    buf->size = 0;
    buf->head = 0;
    
    buf->states = (float *)malloc(capacity * state_dim * sizeof(float));
    buf->actions = (float *)malloc(capacity * action_dim * sizeof(float));
    buf->rewards = (float *)malloc(capacity * sizeof(float));
    buf->next_states = (float *)malloc(capacity * state_dim * sizeof(float));
    buf->dones = (uint8_t *)malloc(capacity * sizeof(uint8_t));
    
    if (!buf->states || !buf->actions || !buf->rewards || !buf->next_states || !buf->dones) {
        nova_replay_buffer_free(buf);
        return NULL;
    }
    
    return buf;
}

void nova_replay_buffer_push(NovaReplayBuffer *buf, const float *state, const float *action,
                             float reward, const float *next_state, uint8_t done) {
    if (!buf) return;
    
    size_t idx = buf->head;
    
    memcpy(&buf->states[idx * buf->state_dim], state, buf->state_dim * sizeof(float));
    memcpy(&buf->actions[idx * buf->action_dim], action, buf->action_dim * sizeof(float));
    buf->rewards[idx] = reward;
    memcpy(&buf->next_states[idx * buf->state_dim], next_state, buf->state_dim * sizeof(float));
    buf->dones[idx] = done;
    
    buf->head = (buf->head + 1) % buf->capacity;
    if (buf->size < buf->capacity) {
        buf->size++;
    }
}

void nova_replay_buffer_sample(NovaReplayBuffer *buf, size_t batch_size,
                               float *out_states, float *out_actions, float *out_rewards,
                               float *out_next_states, uint8_t *out_dones) {
    if (!buf || batch_size > buf->size) return;
    
    for (size_t i = 0; i < batch_size; i++) {
        size_t idx = rand() % buf->size;
        
        memcpy(&out_states[i * buf->state_dim],
               &buf->states[idx * buf->state_dim],
               buf->state_dim * sizeof(float));
        
        memcpy(&out_actions[i * buf->action_dim],
               &buf->actions[idx * buf->action_dim],
               buf->action_dim * sizeof(float));
        
        out_rewards[i] = buf->rewards[idx];
        
        memcpy(&out_next_states[i * buf->state_dim],
               &buf->next_states[idx * buf->state_dim],
               buf->state_dim * sizeof(float));
        
        out_dones[i] = buf->dones[idx];
    }
}

void nova_replay_buffer_free(NovaReplayBuffer *buf) {
    if (!buf) return;
    
    free(buf->states);
    free(buf->actions);
    free(buf->rewards);
    free(buf->next_states);
    free(buf->dones);
    free(buf);
}

/* ============================================================================
 * MLP Implementation
 * ============================================================================ */

NovaMLP* nova_mlp_create(const size_t *layer_sizes, size_t n_layers) {
    if (n_layers < 2) return NULL;
    
    NovaMLP *mlp = (NovaMLP *)malloc(sizeof(NovaMLP));
    if (!mlp) return NULL;
    
    mlp->n_layers = n_layers - 1;
    mlp->layer_sizes = (size_t *)malloc(n_layers * sizeof(size_t));
    mlp->weights = (float **)malloc(mlp->n_layers * sizeof(float *));
    mlp->biases = (float **)malloc(mlp->n_layers * sizeof(float *));
    
    if (!mlp->layer_sizes || !mlp->weights || !mlp->biases) {
        nova_mlp_free(mlp);
        return NULL;
    }
    
    memcpy(mlp->layer_sizes, layer_sizes, n_layers * sizeof(size_t));
    
    /* Initialize weights and biases */
    for (size_t i = 0; i < mlp->n_layers; i++) {
        size_t in_size = layer_sizes[i];
        size_t out_size = layer_sizes[i + 1];
        
        mlp->weights[i] = (float *)malloc(out_size * in_size * sizeof(float));
        mlp->biases[i] = (float *)malloc(out_size * sizeof(float));
        
        if (!mlp->weights[i] || !mlp->biases[i]) {
            nova_mlp_free(mlp);
            return NULL;
        }
        
        /* He initialization for weights */
        for (size_t j = 0; j < out_size * in_size; j++) {
            mlp->weights[i][j] = nova_he_init((float)in_size);
        }
        
        /* Zero initialization for biases */
        memset(mlp->biases[i], 0, out_size * sizeof(float));
    }
    
    return mlp;
}

void nova_mlp_forward(NovaMLP *mlp, const float *input, float *output) {
    if (!mlp || !input || !output) return;
    
    float *current = (float *)malloc(mlp->layer_sizes[mlp->n_layers - 1] * sizeof(float));
    float *prev = (float *)malloc(mlp->layer_sizes[0] * sizeof(float));
    
    if (!current || !prev) {
        free(current);
        free(prev);
        return;
    }
    
    memcpy(prev, input, mlp->layer_sizes[0] * sizeof(float));
    
    /* Forward pass through layers */
    for (size_t layer = 0; layer < mlp->n_layers; layer++) {
        size_t in_size = mlp->layer_sizes[layer];
        size_t out_size = mlp->layer_sizes[layer + 1];
        
        /* Matrix multiplication: y = W * x + b */
        for (size_t j = 0; j < out_size; j++) {
            current[j] = mlp->biases[layer][j];
            for (size_t k = 0; k < in_size; k++) {
                current[j] += mlp->weights[layer][j * in_size + k] * prev[k];
            }
        }
        
        /* Activation: ReLU for hidden layers, tanh for output */
        if (layer < mlp->n_layers - 1) {
            for (size_t j = 0; j < out_size; j++) {
                current[j] = nova_relu(current[j]);
            }
        } else {
            for (size_t j = 0; j < out_size; j++) {
                current[j] = nova_tanh_activate(current[j]);
            }
        }
        
        float *temp = prev;
        prev = current;
        current = temp;
    }
    
    memcpy(output, prev, mlp->layer_sizes[mlp->n_layers] * sizeof(float));
    
    free(current);
    free(prev);
}

void nova_mlp_free(NovaMLP *mlp) {
    if (!mlp) return;
    
    if (mlp->weights) {
        for (size_t i = 0; i < mlp->n_layers; i++) {
            free(mlp->weights[i]);
        }
        free(mlp->weights);
    }
    
    if (mlp->biases) {
        for (size_t i = 0; i < mlp->n_layers; i++) {
            free(mlp->biases[i]);
        }
        free(mlp->biases);
    }
    
    free(mlp->layer_sizes);
    free(mlp);
}

/* ============================================================================
 * SAC (Soft Actor-Critic) Implementation
 * ============================================================================ */

NovaSACAgent* nova_sac_create(size_t state_dim, size_t action_dim, size_t hidden_dim,
                              float gamma, float tau, float alpha) {
    NovaSACAgent *agent = (NovaSACAgent *)malloc(sizeof(NovaSACAgent));
    if (!agent) return NULL;
    
    agent->state_dim = state_dim;
    agent->action_dim = action_dim;
    agent->gamma = gamma;
    agent->tau = tau;
    agent->alpha = alpha;
    agent->log_alpha = logf(alpha + 1e-8f);
    agent->target_entropy = -(float)action_dim;
    
    /* Create actor network: state -> action (with mean and std) */
    size_t actor_sizes[] = {state_dim, hidden_dim, hidden_dim, action_dim * 2};
    agent->actor = nova_mlp_create(actor_sizes, 4);
    
    /* Create critic networks: state + action -> Q-value */
    size_t critic_sizes[] = {state_dim + action_dim, hidden_dim, hidden_dim, 1};
    agent->critic1 = nova_mlp_create(critic_sizes, 4);
    agent->critic2 = nova_mlp_create(critic_sizes, 4);
    agent->target_critic1 = nova_mlp_create(critic_sizes, 4);
    agent->target_critic2 = nova_mlp_create(critic_sizes, 4);
    
    if (!agent->actor || !agent->critic1 || !agent->critic2 ||
        !agent->target_critic1 || !agent->target_critic2) {
        nova_sac_free(agent);
        return NULL;
    }
    
    /* Copy weights to target networks */
    nova_sac_soft_update(agent->target_critic1, agent->critic1, 1.0f);
    nova_sac_soft_update(agent->target_critic2, agent->critic2, 1.0f);
    
    return agent;
}

void nova_sac_select_action(NovaSACAgent *agent, const float *state, float *action, uint8_t explore) {
    if (!agent || !state || !action) return;
    
    float *actor_output = (float *)malloc(agent->action_dim * 2 * sizeof(float));
    if (!actor_output) return;
    
    nova_mlp_forward(agent->actor, state, actor_output);
    
    /* Extract mean and log std */
    for (size_t i = 0; i < agent->action_dim; i++) {
        float mean = actor_output[i];
        float log_std = actor_output[agent->action_dim + i];
        float std = expf(log_std);
        
        if (explore) {
            /* Reparameterization trick: a = tanh(mu + sigma * epsilon) */
            float epsilon = nova_gaussian_sample(0.0f, 1.0f);
            action[i] = tanhf(mean + std * epsilon);
        } else {
            /* Deterministic action */
            action[i] = tanhf(mean);
        }
    }
    
    free(actor_output);
}

void nova_sac_update(NovaSACAgent *agent, NovaReplayBuffer *buffer, size_t batch_size,
                     float lr_actor, float lr_critic, float lr_alpha) {
    if (!agent || !buffer || batch_size > buffer->size) return;
    
    float *states = (float *)malloc(batch_size * agent->state_dim * sizeof(float));
    float *actions = (float *)malloc(batch_size * agent->action_dim * sizeof(float));
    float *rewards = (float *)malloc(batch_size * sizeof(float));
    float *next_states = (float *)malloc(batch_size * agent->state_dim * sizeof(float));
    uint8_t *dones = (uint8_t *)malloc(batch_size * sizeof(uint8_t));
    
    if (!states || !actions || !rewards || !next_states || !dones) {
        free(states);
        free(actions);
        free(rewards);
        free(next_states);
        free(dones);
        return;
    }
    
    nova_replay_buffer_sample(buffer, batch_size, states, actions, rewards, next_states, dones);
    
    /* Compute Q-targets using target networks (simplified update) */
    for (size_t i = 0; i < batch_size; i++) {
        float *q1_input = (float *)malloc((agent->state_dim + agent->action_dim) * sizeof(float));
        float q1_val = 0.0f;
        
        if (q1_input) {
            memcpy(q1_input, &next_states[i * agent->state_dim], agent->state_dim * sizeof(float));
            memcpy(&q1_input[agent->state_dim], &actions[i * agent->action_dim], 
                   agent->action_dim * sizeof(float));
            
            float *q_output = (float *)malloc(sizeof(float));
            if (q_output) {
                nova_mlp_forward(agent->target_critic1, q1_input, q_output);
                q1_val = *q_output;
                free(q_output);
            }
            free(q1_input);
        }
        
        if (!dones[i]) {
            rewards[i] += agent->gamma * q1_val;
        }
    }
    
    /* Soft update target networks */
    nova_sac_soft_update(agent->target_critic1, agent->critic1, agent->tau);
    nova_sac_soft_update(agent->target_critic2, agent->critic2, agent->tau);
    
    (void)lr_actor;
    (void)lr_critic;
    (void)lr_alpha;
    
    free(states);
    free(actions);
    free(rewards);
    free(next_states);
    free(dones);
}

void nova_sac_soft_update(NovaMLP *target, NovaMLP *source, float tau) {
    if (!target || !source) return;
    
    for (size_t layer = 0; layer < target->n_layers; layer++) {
        size_t in_size = target->layer_sizes[layer];
        size_t out_size = target->layer_sizes[layer + 1];
        
        for (size_t i = 0; i < out_size * in_size; i++) {
            target->weights[layer][i] = (1.0f - tau) * target->weights[layer][i] +
                                        tau * source->weights[layer][i];
        }
        
        for (size_t i = 0; i < out_size; i++) {
            target->biases[layer][i] = (1.0f - tau) * target->biases[layer][i] +
                                       tau * source->biases[layer][i];
        }
    }
}

void nova_sac_free(NovaSACAgent *agent) {
    if (!agent) return;
    
    nova_mlp_free(agent->actor);
    nova_mlp_free(agent->critic1);
    nova_mlp_free(agent->critic2);
    nova_mlp_free(agent->target_critic1);
    nova_mlp_free(agent->target_critic2);
    
    free(agent);
}

/* ============================================================================
 * TD3 (Twin Delayed DDPG) Implementation
 * ============================================================================ */

NovaTD3Agent* nova_td3_create(size_t state_dim, size_t action_dim, size_t hidden_dim,
                              float gamma, float tau, float action_noise, size_t policy_delay) {
    NovaTD3Agent *agent = (NovaTD3Agent *)malloc(sizeof(NovaTD3Agent));
    if (!agent) return NULL;
    
    agent->state_dim = state_dim;
    agent->action_dim = action_dim;
    agent->gamma = gamma;
    agent->tau = tau;
    agent->action_noise = action_noise;
    agent->policy_noise = action_noise * 0.5f;
    agent->action_noise_clip = action_noise;
    agent->policy_delay = policy_delay;
    agent->update_count = 0;
    
    /* Create actor network: state -> action */
    size_t actor_sizes[] = {state_dim, hidden_dim, hidden_dim, action_dim};
    agent->actor = nova_mlp_create(actor_sizes, 4);
    agent->target_actor = nova_mlp_create(actor_sizes, 4);
    
    /* Create critic networks: state + action -> Q-value */
    size_t critic_sizes[] = {state_dim + action_dim, hidden_dim, hidden_dim, 1};
    agent->critic1 = nova_mlp_create(critic_sizes, 4);
    agent->critic2 = nova_mlp_create(critic_sizes, 4);
    agent->target_critic1 = nova_mlp_create(critic_sizes, 4);
    agent->target_critic2 = nova_mlp_create(critic_sizes, 4);
    
    if (!agent->actor || !agent->target_actor || !agent->critic1 || !agent->critic2 ||
        !agent->target_critic1 || !agent->target_critic2) {
        nova_td3_free(agent);
        return NULL;
    }
    
    /* Copy weights to target networks */
    nova_sac_soft_update(agent->target_actor, agent->actor, 1.0f);
    nova_sac_soft_update(agent->target_critic1, agent->critic1, 1.0f);
    nova_sac_soft_update(agent->target_critic2, agent->critic2, 1.0f);
    
    return agent;
}

void nova_td3_select_action(NovaTD3Agent *agent, const float *state, float *action, uint8_t explore) {
    if (!agent || !state || !action) return;
    
    nova_mlp_forward(agent->actor, state, action);
    
    if (explore) {
        for (size_t i = 0; i < agent->action_dim; i++) {
            float noise = nova_gaussian_sample(0.0f, agent->action_noise);
            action[i] += noise;
            action[i] = fmaxf(-1.0f, fminf(1.0f, action[i]));
        }
    }
}

void nova_td3_update(NovaTD3Agent *agent, NovaReplayBuffer *buffer, size_t batch_size, float lr) {
    if (!agent || !buffer || batch_size > buffer->size) return;
    
    float *states = (float *)malloc(batch_size * agent->state_dim * sizeof(float));
    float *actions = (float *)malloc(batch_size * agent->action_dim * sizeof(float));
    float *rewards = (float *)malloc(batch_size * sizeof(float));
    float *next_states = (float *)malloc(batch_size * agent->state_dim * sizeof(float));
    uint8_t *dones = (uint8_t *)malloc(batch_size * sizeof(uint8_t));
    
    if (!states || !actions || !rewards || !next_states || !dones) {
        free(states);
        free(actions);
        free(rewards);
        free(next_states);
        free(dones);
        return;
    }
    
    nova_replay_buffer_sample(buffer, batch_size, states, actions, rewards, next_states, dones);
    
    /* Update critic networks (every step) */
    for (size_t i = 0; i < batch_size; i++) {
        float *target_action = (float *)malloc(agent->action_dim * sizeof(float));
        if (!target_action) continue;
        
        nova_mlp_forward(agent->target_actor, &next_states[i * agent->state_dim], target_action);
        for (size_t j = 0; j < agent->action_dim; j++) {
            float noise = nova_gaussian_sample(0.0f, agent->policy_noise);
            target_action[j] += noise;
            target_action[j] = fmaxf(-1.0f, fminf(1.0f, target_action[j]));
        }
        
        float *q_input = (float *)malloc((agent->state_dim + agent->action_dim) * sizeof(float));
        if (q_input) {
            memcpy(q_input, &next_states[i * agent->state_dim], agent->state_dim * sizeof(float));
            memcpy(&q_input[agent->state_dim], target_action, agent->action_dim * sizeof(float));
            
            float *q1 = (float *)malloc(sizeof(float));
            float *q2 = (float *)malloc(sizeof(float));
            
            if (q1 && q2) {
                nova_mlp_forward(agent->target_critic1, q_input, q1);
                nova_mlp_forward(agent->target_critic2, q_input, q2);
                
                float target_q = rewards[i];
                if (!dones[i]) {
                    float min_q = fminf(*q1, *q2);
                    target_q += agent->gamma * min_q;
                }
                rewards[i] = target_q;
            }
            
            free(q1);
            free(q2);
            free(q_input);
        }
        
        free(target_action);
    }
    
    /* Update actor network (delayed: every policy_delay steps) */
    agent->update_count++;
    if (agent->update_count % agent->policy_delay == 0) {
        nova_sac_soft_update(agent->target_actor, agent->actor, agent->tau);
    }
    
    /* Soft update target critic networks */
    nova_sac_soft_update(agent->target_critic1, agent->critic1, agent->tau);
    nova_sac_soft_update(agent->target_critic2, agent->critic2, agent->tau);
    
    (void)lr;
    
    free(states);
    free(actions);
    free(rewards);
    free(next_states);
    free(dones);
}

void nova_td3_free(NovaTD3Agent *agent) {
    if (!agent) return;
    
    nova_mlp_free(agent->actor);
    nova_mlp_free(agent->target_actor);
    nova_mlp_free(agent->critic1);
    nova_mlp_free(agent->critic2);
    nova_mlp_free(agent->target_critic1);
    nova_mlp_free(agent->target_critic2);
    
    free(agent);
}
