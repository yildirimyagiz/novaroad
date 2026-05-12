#include <stdbool.h>

/**
 * @file dimensions.c
 * @brief Dimensional analysis - Physical unit type checking
 */

#include "compiler/dimensions.h"
#include "std/alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* SI base dimensions */
#define DIM_LENGTH      0  /* meter */
#define DIM_MASS        1  /* kilogram */
#define DIM_TIME        2  /* second */
#define DIM_CURRENT     3  /* ampere */
#define DIM_TEMPERATURE 4  /* kelvin */
#define DIM_AMOUNT      5  /* mole */
#define DIM_LUMINOSITY  6  /* candela */
#define NUM_BASE_DIMS   7

/* Dimension structure: stores exponents for each base dimension */
struct nova_dimension {
    int8_t exponents[NUM_BASE_DIMS];
    double scale_factor;  /* For unit conversions (e.g., km = 1000*m) */
};

/* Create dimensionless quantity */
nova_dimension_t *nova_dim_dimensionless(void)
{
    nova_dimension_t *dim = nova_calloc(1, sizeof(nova_dimension_t));
    dim->scale_factor = 1.0;
    return dim;
}

/* Create base dimension */
nova_dimension_t *nova_dim_base(nova_base_dimension_t base)
{
    nova_dimension_t *dim = nova_dim_dimensionless();
    if (base < NUM_BASE_DIMS) {
        dim->exponents[base] = 1;
    }
    return dim;
}

/* Common SI units */
nova_dimension_t *nova_dim_meter(void)      { return nova_dim_base(NOVA_DIM_LENGTH); }
nova_dimension_t *nova_dim_kilogram(void)   { return nova_dim_base(NOVA_DIM_MASS); }
nova_dimension_t *nova_dim_second(void)     { return nova_dim_base(NOVA_DIM_TIME); }
nova_dimension_t *nova_dim_ampere(void)     { return nova_dim_base(NOVA_DIM_CURRENT); }
nova_dimension_t *nova_dim_kelvin(void)     { return nova_dim_base(NOVA_DIM_TEMPERATURE); }

/* Derived units */
nova_dimension_t *nova_dim_newton(void)
{
    /* Newton = kg⋅m⋅s⁻² */
    nova_dimension_t *dim = nova_dim_dimensionless();
    dim->exponents[DIM_MASS] = 1;
    dim->exponents[DIM_LENGTH] = 1;
    dim->exponents[DIM_TIME] = -2;
    return dim;
}

nova_dimension_t *nova_dim_joule(void)
{
    /* Joule = kg⋅m²⋅s⁻² */
    nova_dimension_t *dim = nova_dim_dimensionless();
    dim->exponents[DIM_MASS] = 1;
    dim->exponents[DIM_LENGTH] = 2;
    dim->exponents[DIM_TIME] = -2;
    return dim;
}

nova_dimension_t *nova_dim_watt(void)
{
    /* Watt = kg⋅m²⋅s⁻³ */
    nova_dimension_t *dim = nova_dim_dimensionless();
    dim->exponents[DIM_MASS] = 1;
    dim->exponents[DIM_LENGTH] = 2;
    dim->exponents[DIM_TIME] = -3;
    return dim;
}

nova_dimension_t *nova_dim_volt(void)
{
    /* Volt = kg⋅m²⋅s⁻³⋅A⁻¹ */
    nova_dimension_t *dim = nova_dim_dimensionless();
    dim->exponents[DIM_MASS] = 1;
    dim->exponents[DIM_LENGTH] = 2;
    dim->exponents[DIM_TIME] = -3;
    dim->exponents[DIM_CURRENT] = -1;
    return dim;
}

/* ── Extended SI derived units ─────────────────────────────────────────────── */

/* Pascal: Pa = kg⋅m⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_pascal(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]   =  1;
    d->exponents[NOVA_DIM_LENGTH] = -1;
    d->exponents[NOVA_DIM_TIME]   = -2;
    return d;
}

/* Hertz: Hz = s⁻¹ */
nova_dimension_t *nova_dim_hertz(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_TIME] = -1;
    return d;
}

/* Coulomb: C = A⋅s */
nova_dimension_t *nova_dim_coulomb(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_CURRENT] = 1;
    d->exponents[NOVA_DIM_TIME]    = 1;
    return d;
}

/* Ohm: Ω = kg⋅m²⋅A⁻²⋅s⁻³ */
nova_dimension_t *nova_dim_ohm(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    =  1;
    d->exponents[NOVA_DIM_LENGTH]  =  2;
    d->exponents[NOVA_DIM_CURRENT] = -2;
    d->exponents[NOVA_DIM_TIME]    = -3;
    return d;
}

/* Farad: F = kg⁻¹⋅m⁻²⋅A²⋅s⁴ */
nova_dimension_t *nova_dim_farad(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    = -1;
    d->exponents[NOVA_DIM_LENGTH]  = -2;
    d->exponents[NOVA_DIM_CURRENT] =  2;
    d->exponents[NOVA_DIM_TIME]    =  4;
    return d;
}

/* Tesla: T = kg⋅A⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_tesla(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    =  1;
    d->exponents[NOVA_DIM_CURRENT] = -1;
    d->exponents[NOVA_DIM_TIME]    = -2;
    return d;
}

/* Henry: H = kg⋅m²⋅A⁻²⋅s⁻² */
nova_dimension_t *nova_dim_henry(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    =  1;
    d->exponents[NOVA_DIM_LENGTH]  =  2;
    d->exponents[NOVA_DIM_CURRENT] = -2;
    d->exponents[NOVA_DIM_TIME]    = -2;
    return d;
}

/* Weber: Wb = kg⋅m²⋅A⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_weber(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    =  1;
    d->exponents[NOVA_DIM_LENGTH]  =  2;
    d->exponents[NOVA_DIM_CURRENT] = -1;
    d->exponents[NOVA_DIM_TIME]    = -2;
    return d;
}

/* Siemens: S = kg⁻¹⋅m⁻²⋅A²⋅s³ */
nova_dimension_t *nova_dim_siemens(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_MASS]    = -1;
    d->exponents[NOVA_DIM_LENGTH]  = -2;
    d->exponents[NOVA_DIM_CURRENT] =  2;
    d->exponents[NOVA_DIM_TIME]    =  3;
    return d;
}

/* Lumen: lm = cd (sr is dimensionless) */
nova_dimension_t *nova_dim_lumen(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_LUMINOSITY] = 1;
    return d;
}

/* Lux: lx = cd⋅m⁻² */
nova_dimension_t *nova_dim_lux(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_LUMINOSITY] =  1;
    d->exponents[NOVA_DIM_LENGTH]     = -2;
    return d;
}

/* Becquerel: Bq = s⁻¹ (same dim as Hz) */
nova_dimension_t *nova_dim_becquerel(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_TIME] = -1;
    return d;
}

/* Gray: Gy = m²⋅s⁻² (J/kg) */
nova_dimension_t *nova_dim_gray(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_LENGTH] =  2;
    d->exponents[NOVA_DIM_TIME]   = -2;
    return d;
}

/* Sievert: Sv = m²⋅s⁻² (same dim as Gy) */
nova_dimension_t *nova_dim_sievert(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_LENGTH] =  2;
    d->exponents[NOVA_DIM_TIME]   = -2;
    return d;
}

/* Katal: kat = mol⋅s⁻¹ */
nova_dimension_t *nova_dim_katal(void)
{
    nova_dimension_t *d = nova_dim_dimensionless();
    d->exponents[NOVA_DIM_AMOUNT] =  1;
    d->exponents[NOVA_DIM_TIME]   = -1;
    return d;
}

/* Multiplication: [L¹M¹T⁻²] * [L²] = [L³M¹T⁻²] */
nova_dimension_t *nova_dim_multiply(const nova_dimension_t *a, const nova_dimension_t *b)
{
    nova_dimension_t *result = nova_dim_dimensionless();
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        result->exponents[i] = a->exponents[i] + b->exponents[i];
    }
    
    result->scale_factor = a->scale_factor * b->scale_factor;
    
    return result;
}

/* Division: [L²M¹T⁻²] / [T¹] = [L²M¹T⁻³] */
nova_dimension_t *nova_dim_divide(const nova_dimension_t *a, const nova_dimension_t *b)
{
    nova_dimension_t *result = nova_dim_dimensionless();
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        result->exponents[i] = a->exponents[i] - b->exponents[i];
    }
    
    result->scale_factor = a->scale_factor / b->scale_factor;
    
    return result;
}

/* Power: [L¹M¹T⁻²]² = [L²M²T⁻⁴] */
nova_dimension_t *nova_dim_power(const nova_dimension_t *dim, int exponent)
{
    nova_dimension_t *result = nova_dim_dimensionless();
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        result->exponents[i] = dim->exponents[i] * exponent;
    }
    
    result->scale_factor = pow(dim->scale_factor, exponent);
    
    return result;
}

/* Check compatibility (same dimensions) */
bool nova_dim_compatible(const nova_dimension_t *a, const nova_dimension_t *b)
{
    if (!a || !b) return false;
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        if (a->exponents[i] != b->exponents[i]) {
            return false;
        }
    }
    
    return true;
}

/* Check if dimensionless */
bool nova_dim_is_dimensionless(const nova_dimension_t *dim)
{
    if (!dim) return false;
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        if (dim->exponents[i] != 0) {
            return false;
        }
    }
    
    return true;
}

/* Convert to string representation */
const char *nova_dim_to_string(const nova_dimension_t *dim)
{
    static char buffer[256];
    buffer[0] = '\0';
    
    const char *base_names[] = {"m", "kg", "s", "A", "K", "mol", "cd"};
    bool first = true;
    
    for (int i = 0; i < NUM_BASE_DIMS; i++) {
        if (dim->exponents[i] != 0) {
            if (!first) strcat(buffer, "⋅");
            first = false;
            
            strcat(buffer, base_names[i]);
            
            if (dim->exponents[i] != 1) {
                char exp[16];
                snprintf(exp, sizeof(exp), "^%d", dim->exponents[i]);
                strcat(buffer, exp);
            }
        }
    }
    
    if (first) {
        strcpy(buffer, "dimensionless");
    }
    
    return buffer;
}

/* Parse dimension from string (e.g., "kg", "m/s", "kg⋅m⋅s^-2") */
/* Forward declarations for SI prefix helpers defined later in this file */
static double nova_dim_si_prefix_scale(const char *prefix);
static nova_dimension_t *nova_dim_si_base_unit(const char *unit);
static nova_dimension_t *nova_dim_parse_prefixed(const char *token);

nova_dimension_t *nova_dim_parse(const char *str)
{
    if (!str || !*str) {
        return nova_dim_dimensionless();
    }

    /* Fast path: temperature units with degree sign (°C, °F) — handle before tokenization
     * because '°' (C2 B0) is a 2-byte UTF-8 sequence that confuses the prefix splitter.
     * "°C" = \xC2\xB0\x43 = 3 bytes. "°F" = \xC2\xB0\x46 = 3 bytes. */
    if (strncmp(str, "\xC2\xB0""C", 3) == 0) return nova_dim_kelvin(); /* °C */
    if (strncmp(str, "\xC2\xB0""F", 3) == 0) return nova_dim_kelvin(); /* °F */
    if (strcmp(str, "degC") == 0 || strcmp(str, "Celsius")    == 0) return nova_dim_kelvin();
    if (strcmp(str, "degF") == 0 || strcmp(str, "Fahrenheit") == 0) return nova_dim_kelvin();

    nova_dimension_t *result = nova_dim_dimensionless();
    char buffer[64];
    int buf_idx = 0;
    const char *p = str;
    
    while (*p) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        
        // Read unit name (letters only — stop at digit, '^', '/', '*', space, unicode dot)
        buf_idx = 0;
        while (*p && *p != '/' && *p != '*' && *p != '^' &&
               *p != ' ' && buf_idx < 63 &&
               !(*p >= '0' && *p <= '9') &&              /* stop at digit (trailing exponent like m2) */
               !(p[0] == '\xC2' && p[1] == '\xB7') &&    /* UTF-8 U+00B7 MIDDLE DOT */
               !(p[0] == '\xE2' && p[1] == '\x8B' && p[2] == '\x85')) { /* UTF-8 U+22C5 ⋅ */
            buffer[buf_idx++] = *p++;
        }
        buffer[buf_idx] = '\0';

        // Parse exponent if present (^N, N, or -N directly after unit name)
        int exponent = 1;
        if (*p == '^' || (*p >= '0' && *p <= '9') || *p == '-') {
            if (*p == '^') p++;
            
            int sign = 1;
            if (*p == '-') {
                sign = -1;
                p++;
            }
            
            if (*p >= '0' && *p <= '9') {
                exponent = (*p - '0') * sign;
                p++;
                // Handle multi-digit exponents
                while (*p >= '0' && *p <= '9') {
                    exponent = exponent * 10 + (*p - '0') * sign;
                    p++;
                }
            }
        }
        
        // Match unit name and update dimensions
        nova_dimension_t *unit_dim = NULL;
        
        // SI base units
        if (strcmp(buffer, "m") == 0) {
            unit_dim = nova_dim_meter();
        } else if (strcmp(buffer, "kg") == 0) {
            unit_dim = nova_dim_kilogram();
        } else if (strcmp(buffer, "g") == 0) {
            unit_dim = nova_dim_kilogram();
            unit_dim->scale_factor = 0.001;  // gram = 0.001 kg
        } else if (strcmp(buffer, "s") == 0) {
            unit_dim = nova_dim_second();
        } else if (strcmp(buffer, "A") == 0) {
            unit_dim = nova_dim_ampere();
        } else if (strcmp(buffer, "K") == 0) {
            unit_dim = nova_dim_kelvin();
        } else if (strcmp(buffer, "mol") == 0) {
            unit_dim = nova_dim_base(NOVA_DIM_AMOUNT);
        } else if (strcmp(buffer, "cd") == 0) {
            unit_dim = nova_dim_base(NOVA_DIM_LUMINOSITY);
        }
        // Derived units
        else if (strcmp(buffer, "N") == 0) {
            unit_dim = nova_dim_newton();
        } else if (strcmp(buffer, "J") == 0) {
            unit_dim = nova_dim_joule();
        } else if (strcmp(buffer, "W") == 0) {
            unit_dim = nova_dim_watt();
        } else if (strcmp(buffer, "V") == 0) {
            unit_dim = nova_dim_volt();
        }
        // Extended SI derived units
        else if (strcmp(buffer, "Pa") == 0) {
            unit_dim = nova_dim_pascal();
        } else if (strcmp(buffer, "Hz") == 0) {
            unit_dim = nova_dim_hertz();
        } else if (strcmp(buffer, "C") == 0) {
            unit_dim = nova_dim_coulomb();
        } else if (strcmp(buffer, "Ohm") == 0 || strcmp(buffer, "\xce\xa9") == 0) {
            /* Ohm: ASCII "Ohm" or Unicode Ω (U+03A9, UTF-8: 0xCE 0xA9) */
            unit_dim = nova_dim_ohm();
        } else if (strcmp(buffer, "F") == 0) {
            unit_dim = nova_dim_farad();
        } else if (strcmp(buffer, "T") == 0) {
            unit_dim = nova_dim_tesla();
        } else if (strcmp(buffer, "H") == 0) {
            unit_dim = nova_dim_henry();
        } else if (strcmp(buffer, "Wb") == 0) {
            unit_dim = nova_dim_weber();
        } else if (strcmp(buffer, "S") == 0) {
            unit_dim = nova_dim_siemens();
        } else if (strcmp(buffer, "lm") == 0) {
            unit_dim = nova_dim_lumen();
        } else if (strcmp(buffer, "lx") == 0) {
            unit_dim = nova_dim_lux();
        } else if (strcmp(buffer, "Bq") == 0) {
            unit_dim = nova_dim_becquerel();
        } else if (strcmp(buffer, "Gy") == 0) {
            unit_dim = nova_dim_gray();
        } else if (strcmp(buffer, "Sv") == 0) {
            unit_dim = nova_dim_sievert();
        } else if (strcmp(buffer, "kat") == 0) {
            unit_dim = nova_dim_katal();
        }
        // Non-SI but common — must be listed BEFORE prefix fallback
        else if (strcmp(buffer, "min") == 0) {
            unit_dim = nova_dim_second();
            unit_dim->scale_factor = 60.0;
        } else if (strcmp(buffer, "h") == 0) {
            unit_dim = nova_dim_second();
            unit_dim->scale_factor = 3600.0;
        } else if (strcmp(buffer, "d") == 0) {  /* day */
            unit_dim = nova_dim_second();
            unit_dim->scale_factor = 86400.0;
        } else if (strcmp(buffer, "bar") == 0) {
            unit_dim = nova_dim_pascal();
            unit_dim->scale_factor = 1e5;
        } else if (strcmp(buffer, "eV") == 0) {
            unit_dim = nova_dim_joule();
            unit_dim->scale_factor = 1.602176634e-19;
        } else if (strcmp(buffer, "au") == 0) {  /* astronomical unit */
            unit_dim = nova_dim_meter();
            unit_dim->scale_factor = 1.495978707e11;
        } else if (strcmp(buffer, "ly") == 0) {  /* light-year */
            unit_dim = nova_dim_meter();
            unit_dim->scale_factor = 9.4607304725808e15;
        } else if (strcmp(buffer, "pc") == 0) {  /* parsec */
            unit_dim = nova_dim_meter();
            unit_dim->scale_factor = 3.085677581491367e16;
        } else if (strcmp(buffer, "atm") == 0) {  /* atmosphere */
            unit_dim = nova_dim_pascal();
            unit_dim->scale_factor = 101325.0;
        } else if (strcmp(buffer, "cal") == 0) {  /* calorie */
            unit_dim = nova_dim_joule();
            unit_dim->scale_factor = 4.184;
        } else if (strcmp(buffer, "kcal") == 0) {  /* kilocalorie */
            unit_dim = nova_dim_joule();
            unit_dim->scale_factor = 4184.0;
        } else if (strcmp(buffer, "Wh") == 0) {  /* watt-hour */
            unit_dim = nova_dim_joule();
            unit_dim->scale_factor = 3600.0;
        } else if (strcmp(buffer, "kWh") == 0) {  /* kilowatt-hour */
            unit_dim = nova_dim_joule();
            unit_dim->scale_factor = 3.6e6;
        }
        // ── SI prefix + base unit fallback (MPa, kHz, mV, nA, μF, GHz, etc.) ──
        else {
            unit_dim = nova_dim_parse_prefixed(buffer);
            /* If still unknown, use dimensionless (best-effort, no hard error) */
            if (!unit_dim) unit_dim = nova_dim_dimensionless();
        }
        /* Non-SI time units not handled by prefix table */
        if (unit_dim == NULL && strcmp(buffer, "min") == 0) {
            unit_dim = nova_dim_second();
            unit_dim->scale_factor = 60.0;
        }
        /* Safety fallback: should not reach here if parse table and prefix table are complete */
        if (unit_dim == NULL) {
            unit_dim = nova_dim_dimensionless();
        }
        
        // Apply exponent
        if (exponent != 1 && unit_dim) {
            nova_dimension_t *powered = nova_dim_power(unit_dim, exponent);
            nova_dim_destroy(unit_dim);
            unit_dim = powered;
        }
        
        // Combine with result
        if (unit_dim) {
            nova_dimension_t *combined = nova_dim_multiply(result, unit_dim);
            nova_dim_destroy(result);
            nova_dim_destroy(unit_dim);
            result = combined;
        }
        
        // Skip operator
        if (*p == '*') {
            p++;
        } else if (p[0] == '\xC2' && p[1] == '\xB7') {  // UTF-8 U+00B7 MIDDLE DOT
            p += 2;
        } else if (p[0] == '\xE2' && p[1] == '\x8B' && p[2] == '\x85') {  // UTF-8 U+22C5 DOT OPERATOR ⋅
            p += 3;
        } else if (*p == '/') {
            // Division - next unit gets negative exponent
            p++;
            // Read next unit and apply with negative exponent
            buf_idx = 0;
            while (*p && *p != '*' && *p != '/' && 
                   *p != ' ' && buf_idx < 63 &&
                   !(p[0] == '\xC2' && p[1] == '\xB7') &&  // UTF-8 U+00B7
                   !(p[0] == '\xE2' && p[1] == '\x8B' && p[2] == '\x85')) {  // UTF-8 U+22C5 ⋅
                buffer[buf_idx++] = *p++;
            }
            buffer[buf_idx] = '\0';
            
            nova_dimension_t *divisor = NULL;
            /* Parse exponent after divisor unit (e.g. m/s2 → m·s⁻²) */
            int div_exponent = 1;
            char *exp_ptr = buffer + strlen(buffer);
            /* Check for trailing digits as exponent */
            char *digit_start = buffer;
            while (*digit_start && !(*digit_start >= '0' && *digit_start <= '9') &&
                   *digit_start != '^')
                digit_start++;
            if (*digit_start == '^') {
                *digit_start = '\0';
                div_exponent = atoi(digit_start + 1);
            } else if (*digit_start >= '0' && *digit_start <= '9') {
                div_exponent = atoi(digit_start);
                *digit_start = '\0';
            }
            (void)exp_ptr;

            /* Full set of SI base and common derived units for divisor */
            if (strcmp(buffer, "m") == 0) {
                divisor = nova_dim_meter();
            } else if (strcmp(buffer, "kg") == 0) {
                divisor = nova_dim_kilogram();
            } else if (strcmp(buffer, "g") == 0) {
                divisor = nova_dim_kilogram();
                divisor->scale_factor = 0.001;
            } else if (strcmp(buffer, "s") == 0) {
                divisor = nova_dim_second();
            } else if (strcmp(buffer, "A") == 0) {
                divisor = nova_dim_ampere();
            } else if (strcmp(buffer, "K") == 0) {
                divisor = nova_dim_kelvin();
            } else if (strcmp(buffer, "mol") == 0) {
                divisor = nova_dim_base(NOVA_DIM_AMOUNT);
            } else if (strcmp(buffer, "cd") == 0) {
                divisor = nova_dim_base(NOVA_DIM_LUMINOSITY);
            } else if (strcmp(buffer, "N") == 0) {
                divisor = nova_dim_newton();
            } else if (strcmp(buffer, "J") == 0) {
                divisor = nova_dim_joule();
            } else if (strcmp(buffer, "W") == 0) {
                divisor = nova_dim_watt();
            } else if (strcmp(buffer, "V") == 0) {
                divisor = nova_dim_volt();
            } else if (strcmp(buffer, "Pa") == 0) {
                divisor = nova_dim_pascal();
            } else if (strcmp(buffer, "Hz") == 0) {
                divisor = nova_dim_hertz();
            } else if (strcmp(buffer, "C") == 0) {
                divisor = nova_dim_coulomb();
            } else if (strcmp(buffer, "Ohm") == 0 || strcmp(buffer, "\xce\xa9") == 0) {
                divisor = nova_dim_ohm();
            } else if (strcmp(buffer, "F") == 0) {
                divisor = nova_dim_farad();
            } else if (strcmp(buffer, "T") == 0) {
                divisor = nova_dim_tesla();
            } else if (strcmp(buffer, "H") == 0) {
                divisor = nova_dim_henry();
            } else if (strcmp(buffer, "Wb") == 0) {
                divisor = nova_dim_weber();
            } else if (strcmp(buffer, "S") == 0) {
                divisor = nova_dim_siemens();
            } else if (strcmp(buffer, "lm") == 0) {
                divisor = nova_dim_lumen();
            } else if (strcmp(buffer, "lx") == 0) {
                divisor = nova_dim_lux();
            } else if (strcmp(buffer, "Bq") == 0) {
                divisor = nova_dim_becquerel();
            } else if (strcmp(buffer, "Gy") == 0) {
                divisor = nova_dim_gray();
            } else if (strcmp(buffer, "Sv") == 0) {
                divisor = nova_dim_sievert();
            } else if (strcmp(buffer, "kat") == 0) {
                divisor = nova_dim_katal();
            } else if (strcmp(buffer, "km") == 0) {
                divisor = nova_dim_meter();
                divisor->scale_factor = 1000.0;
            } else if (strcmp(buffer, "cm") == 0) {
                divisor = nova_dim_meter();
                divisor->scale_factor = 0.01;
            } else if (strcmp(buffer, "mm") == 0) {
                divisor = nova_dim_meter();
                divisor->scale_factor = 0.001;
            } else if (strcmp(buffer, "ms") == 0) {
                divisor = nova_dim_second();
                divisor->scale_factor = 1e-3;
            } else if (strcmp(buffer, "us") == 0 || strcmp(buffer, "\xce\xbcs") == 0) {
                divisor = nova_dim_second();
                divisor->scale_factor = 1e-6;
            } else if (strcmp(buffer, "ns") == 0) {
                divisor = nova_dim_second();
                divisor->scale_factor = 1e-9;
            } else if (strcmp(buffer, "min") == 0) {
                divisor = nova_dim_second();
                divisor->scale_factor = 60.0;
            } else if (strcmp(buffer, "h") == 0) {
                divisor = nova_dim_second();
                divisor->scale_factor = 3600.0;
            } else {
                divisor = nova_dim_dimensionless();
            }

            /* Apply exponent to divisor if needed (e.g. m/s2 means s^2 in denominator) */
            if (div_exponent != 1) {
                nova_dimension_t *powered = nova_dim_power(divisor, div_exponent);
                nova_dim_destroy(divisor);
                divisor = powered;
            }
            
            nova_dimension_t *combined = nova_dim_divide(result, divisor);
            nova_dim_destroy(result);
            nova_dim_destroy(divisor);
            result = combined;
        }
    }
    
    return result;
}

/* Create custom dimension with scale */
nova_dimension_t *nova_dim_with_scale(const nova_dimension_t *base, double scale)
{
    nova_dimension_t *dim = nova_alloc(sizeof(nova_dimension_t));
    memcpy(dim, base, sizeof(nova_dimension_t));
    dim->scale_factor = scale;
    return dim;
}

/* Common unit conversions */
double nova_dim_convert(double value, const nova_dimension_t *from, const nova_dimension_t *to)
{
    if (!nova_dim_compatible(from, to)) {
        return NAN;  /* Incompatible dimensions */
    }
    
    return value * (from->scale_factor / to->scale_factor);
}

/* Destroy dimension */
void nova_dim_destroy(nova_dimension_t *dim)
{
    nova_free(dim);
}

/* Get scale factor */
double nova_dim_get_scale(const nova_dimension_t *dim)
{
    if (!dim) return 1.0;
    return dim->scale_factor;
}

/* ── SI Prefix Table ─────────────────────────────────────────────────────────
 * Resolves standard metric prefixes (Y, Z, E, P, T, G, M, k, h, da,
 *                                     d, c, m, u/μ, n, p, f, a, z, y)
 * Returns the scale factor for the prefix, or 0.0 if not a known prefix.
 * The prefix string must be a single token extracted from the unit name.
 */
static double nova_dim_si_prefix_scale(const char *prefix)
{
    /* Map: prefix string → power of 10 scale */
    static const struct { const char *sym; double scale; } PREFIXES[] = {
        /* Large */
        { "Y",    1e24  },  /* yotta */
        { "Z",    1e21  },  /* zetta */
        { "E",    1e18  },  /* exa   */
        { "P",    1e15  },  /* peta  */
        { "T",    1e12  },  /* tera  */
        { "G",    1e9   },  /* giga  */
        { "M",    1e6   },  /* mega  */
        { "k",    1e3   },  /* kilo  */
        { "h",    1e2   },  /* hecto */
        { "da",   1e1   },  /* deca  */
        /* Small */
        { "d",    1e-1  },  /* deci  */
        { "c",    1e-2  },  /* centi */
        { "m",    1e-3  },  /* milli */
        /* UTF-8 μ (U+03BC): 0xCE 0xBC */
        { "\xce\xbc", 1e-6  },
        /* ASCII 'u' as fallback for micro */
        { "u",    1e-6  },
        { "n",    1e-9  },  /* nano  */
        { "p",    1e-12 },  /* pico  */
        { "f",    1e-15 },  /* femto */
        { "a",    1e-18 },  /* atto  */
        { "z",    1e-21 },  /* zepto */
        { "y",    1e-24 },  /* yocto */
    };
    static const int N_PREFIXES = (int)(sizeof(PREFIXES) / sizeof(PREFIXES[0]));

    for (int i = 0; i < N_PREFIXES; i++) {
        if (strcmp(prefix, PREFIXES[i].sym) == 0)
            return PREFIXES[i].scale;
    }
    return 0.0; /* not a known prefix */
}

/* SI base + derived units that can accept metric prefixes.
 * Returns a heap-allocated nova_dimension_t with scale=1.0, or NULL if unknown. */
static nova_dimension_t *nova_dim_si_base_unit(const char *unit)
{
    /* SI base units */
    if (strcmp(unit, "m")   == 0) return nova_dim_meter();
    if (strcmp(unit, "g")   == 0) { nova_dimension_t *d = nova_dim_kilogram(); d->scale_factor = 1e-3; return d; }
    if (strcmp(unit, "s")   == 0) return nova_dim_second();
    if (strcmp(unit, "A")   == 0) return nova_dim_ampere();
    if (strcmp(unit, "K")   == 0) return nova_dim_kelvin();
    /* Temperature units (offset-based, all map to TEMPERATURE dimension) */
    if (strcmp(unit, "\xC2\xB0""C") == 0) return nova_dim_kelvin(); /* °C */
    if (strcmp(unit, "\xC2\xB0""F") == 0) return nova_dim_kelvin(); /* °F */
    if (strcmp(unit, "degC") == 0) return nova_dim_kelvin();
    if (strcmp(unit, "degF") == 0) return nova_dim_kelvin();
    if (strcmp(unit, "Celsius")    == 0) return nova_dim_kelvin();
    if (strcmp(unit, "Fahrenheit") == 0) return nova_dim_kelvin();
    if (strcmp(unit, "mol") == 0) { nova_dimension_t *d = nova_dim_dimensionless(); d->exponents[NOVA_DIM_AMOUNT] = 1; return d; }
    if (strcmp(unit, "cd")  == 0) { nova_dimension_t *d = nova_dim_dimensionless(); d->exponents[NOVA_DIM_LUMINOSITY] = 1; return d; }
    /* SI derived units */
    if (strcmp(unit, "N")   == 0) return nova_dim_newton();
    if (strcmp(unit, "J")   == 0) return nova_dim_joule();
    if (strcmp(unit, "W")   == 0) return nova_dim_watt();
    if (strcmp(unit, "V")   == 0) return nova_dim_volt();
    if (strcmp(unit, "Pa")  == 0) return nova_dim_pascal();
    if (strcmp(unit, "Hz")  == 0) return nova_dim_hertz();
    if (strcmp(unit, "C")   == 0) return nova_dim_coulomb();
    if (strcmp(unit, "Ohm") == 0) return nova_dim_ohm();
    if (strcmp(unit, "F")   == 0) return nova_dim_farad();
    if (strcmp(unit, "T")   == 0) return nova_dim_tesla();
    if (strcmp(unit, "H")   == 0) return nova_dim_henry();
    if (strcmp(unit, "Wb")  == 0) return nova_dim_weber();
    if (strcmp(unit, "S")   == 0) return nova_dim_siemens();
    if (strcmp(unit, "lm")  == 0) return nova_dim_lumen();
    if (strcmp(unit, "lx")  == 0) return nova_dim_lux();
    if (strcmp(unit, "Bq")  == 0) return nova_dim_becquerel();
    if (strcmp(unit, "Gy")  == 0) return nova_dim_gray();
    if (strcmp(unit, "Sv")  == 0) return nova_dim_sievert();
    if (strcmp(unit, "kat") == 0) return nova_dim_katal();
    /* Common non-SI units usable with prefixes */
    if (strcmp(unit, "eV")  == 0) { nova_dimension_t *d = nova_dim_joule();  d->scale_factor = 1.602176634e-19; return d; }
    if (strcmp(unit, "bar") == 0) { nova_dimension_t *d = nova_dim_pascal(); d->scale_factor = 1e5; return d; }
    if (strcmp(unit, "cal") == 0) { nova_dimension_t *d = nova_dim_joule();  d->scale_factor = 4.184; return d; }
    if (strcmp(unit, "Wh")  == 0) { nova_dimension_t *d = nova_dim_joule();  d->scale_factor = 3600.0; return d; }
    return NULL;
}

/**
 * nova_dim_parse_prefixed: Try to split `token` into SI prefix + base unit.
 * e.g. "MPa" → prefix="M" (1e6) + base="Pa" → Pa * 1e6
 *      "kHz" → prefix="k" (1e3) + base="Hz" → Hz * 1e3
 *      "mV"  → prefix="m" (1e-3)+ base="V"  → V  * 1e-3
 *      "nA"  → prefix="n" (1e-9)+ base="A"  → A  * 1e-9
 *      "μF"  → prefix="μ" (1e-6)+ base="F"  → F  * 1e-6
 *
 * Returns a new dimension with correct scale, or NULL if not a valid prefix+unit.
 * Multi-byte prefix (μ, U+03BC) is handled via the UTF-8 two-byte sequence.
 */
static nova_dimension_t *nova_dim_parse_prefixed(const char *token)
{
    size_t len = strlen(token);
    if (len < 2) return NULL;

    /* Try two-byte UTF-8 prefix first (μ = 0xCE 0xBC) */
    if (len >= 3 &&
        (unsigned char)token[0] == 0xCE &&
        (unsigned char)token[1] == 0xBC) {
        /* μ prefix — base unit starts at token+2 */
        nova_dimension_t *base = nova_dim_si_base_unit(token + 2);
        if (base) {
            base->scale_factor *= 1e-6;
            return base;
        }
        return NULL;
    }

    /* Try single-character prefix — order: longest first to avoid 'da' vs 'd' clash */
    /* Two-char prefix "da" (deca) */
    if (len >= 3 && token[0] == 'd' && token[1] == 'a') {
        nova_dimension_t *base = nova_dim_si_base_unit(token + 2);
        if (base) { base->scale_factor *= 1e1; return base; }
    }

    /* Single-char prefix */
    char prefix_buf[3] = { token[0], '\0', '\0' };
    double scale = nova_dim_si_prefix_scale(prefix_buf);
    if (scale != 0.0) {
        nova_dimension_t *base = nova_dim_si_base_unit(token + 1);
        if (base) {
            base->scale_factor *= scale;
            return base;
        }
    }

    return NULL; /* not a valid prefix+unit */
}
