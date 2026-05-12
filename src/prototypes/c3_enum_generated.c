// Generated code for C.3 enums

#include <stdint.h>
#include <stdbool.h>

// Tagged union for Option
typedef struct nova_Option {
    uint32_t discriminant;
    union {
        struct {
            void* field0;
        } Some;
    } payload;
} nova_Option;

// Enum constructors for Option
nova_Option nova_Option_Some(void* arg0) {
    nova_Option result;
    result.discriminant = 0;
    result.payload.Some.field0 = arg0;
    return result;
}

nova_Option nova_Option_None(void) {
    nova_Option result;
    result.discriminant = 1;
    return result;
}

// is_variant helpers for Option
bool nova_Option_is_Some(nova_Option* value) {
    return value->discriminant == 0;
}

bool nova_Option_is_None(nova_Option* value) {
    return value->discriminant == 1;
}

// Tagged union for Result
typedef struct nova_Result {
    uint32_t discriminant;
    union {
        struct {
            void* field0;
        } Ok;
        struct {
            void* field0;
        } Err;
    } payload;
} nova_Result;

// Enum constructors for Result
nova_Result nova_Result_Ok(void* arg0) {
    nova_Result result;
    result.discriminant = 0;
    result.payload.Ok.field0 = arg0;
    return result;
}

nova_Result nova_Result_Err(void* arg0) {
    nova_Result result;
    result.discriminant = 1;
    result.payload.Err.field0 = arg0;
    return result;
}

// is_variant helpers for Result
bool nova_Result_is_Ok(nova_Result* value) {
    return value->discriminant == 0;
}

bool nova_Result_is_Err(nova_Result* value) {
    return value->discriminant == 1;
}

