// Generated code for enums

#include <stdint.h>

// Tagged union for Option
typedef struct Option {
    uint32_t discriminant;
    union {
        struct {
            void* field0;
        } Some;
    } payload;
} Option;

// Enum constructors for Option
Option Option_Some(void* arg0) {
    Option result;
    result.discriminant = 0;
    result.payload.Some.field0 = arg0;
    return result;
}

Option Option_None(void) {
    Option result;
    result.discriminant = 1;
    return result;
}

// Tagged union for Result
typedef struct Result {
    uint32_t discriminant;
    union {
        struct {
            void* field0;
        } Ok;
        struct {
            void* field0;
        } Err;
    } payload;
} Result;

// Enum constructors for Result
Result Result_Ok(void* arg0) {
    Result result;
    result.discriminant = 0;
    result.payload.Ok.field0 = arg0;
    return result;
}

Result Result_Err(void* arg0) {
    Result result;
    result.discriminant = 1;
    result.payload.Err.field0 = arg0;
    return result;
}

