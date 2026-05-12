// Pattern matching examples

// Pattern match: Option::Some
if (enum_value.discriminant == 0) {
    // Extract payload for Some
    // Pattern matching logic here
}

// Pattern match: Option::None
if (enum_value.discriminant == 1) {
    // Extract payload for None
    // Pattern matching logic here
}

// Pattern match: Result::Ok
if (enum_value.discriminant == 0) {
    // Extract payload for Ok
    // Pattern matching logic here
}

// Pattern match: Result::Err
if (enum_value.discriminant == 1) {
    // Extract payload for Err
    // Pattern matching logic here
}
