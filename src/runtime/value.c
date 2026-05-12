/**
 * @file value.c
 * @brief Runtime value representation
 */

#include "runtime/value.h"

nova_value_t nova_value_nil(void)
{
    nova_value_t val;
    val.type = NOVA_VAL_NIL;
    return val;
}

nova_value_t nova_value_bool(bool b)
{
    nova_value_t val;
    val.type = NOVA_VAL_BOOL;
    val.as.boolean = b;
    return val;
}

nova_value_t nova_value_int(int64_t i)
{
    nova_value_t val;
    val.type = NOVA_VAL_INT;
    val.as.integer = i;
    return val;
}

nova_value_t nova_value_float(double f)
{
    nova_value_t val;
    val.type = NOVA_VAL_FLOAT;
    val.as.floating = f;
    return val;
}

bool nova_value_is_type(nova_value_t val, nova_value_type_t type)
{
    return val.type == type;
}

int64_t nova_value_as_int(nova_value_t val)
{
    return val.as.integer;
}

double nova_value_as_float(nova_value_t val)
{
    return val.as.floating;
}
