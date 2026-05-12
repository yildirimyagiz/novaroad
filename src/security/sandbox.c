/**
 * @file sandbox.c
 */

#include "security/sandbox.h"
#include "std/alloc.h"

struct nova_sandbox {
    nova_sandbox_config_t config;
};

nova_sandbox_t *nova_sandbox_create(const nova_sandbox_config_t *config)
{
    nova_sandbox_t *sandbox = nova_alloc(sizeof(nova_sandbox_t));
    if (sandbox) {
        sandbox->config = *config;
    }
    return sandbox;
}

int nova_sandbox_enter(nova_sandbox_t *sandbox)
{
    (void)sandbox;
    return 0;
}

void nova_sandbox_exit(nova_sandbox_t *sandbox)
{
    (void)sandbox;
}

void nova_sandbox_destroy(nova_sandbox_t *sandbox)
{
    nova_free(sandbox);
}
