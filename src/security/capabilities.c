/**
 * @file capabilities.c
 */

#include "security/capabilities.h"

static nova_capability_t current_caps = 0;

bool nova_has_capability(nova_capability_t cap)
{
    return (current_caps & cap) != 0;
}

int nova_grant_capability(nova_capability_t cap)
{
    current_caps |= cap;
    return 0;
}

int nova_revoke_capability(nova_capability_t cap)
{
    current_caps &= ~cap;
    return 0;
}

nova_capability_t nova_get_capabilities(void)
{
    return current_caps;
}
