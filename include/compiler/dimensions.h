/**
 * @file dimensions.h
 * @brief Dimensional analysis for physical units
 * 
 * Enables compile-time checking of physical units:
 * - 5.kg * 3.m/s² = 15.N (force)
 * - 5.kg + 3.m => TYPE ERROR
 */

#ifndef NOVA_DIMENSIONS_H
#define NOVA_DIMENSIONS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nova_dimension nova_dimension_t;

/* Base SI dimensions */
typedef enum {
    NOVA_DIM_LENGTH,
    NOVA_DIM_MASS,
    NOVA_DIM_TIME,
    NOVA_DIM_CURRENT,
    NOVA_DIM_TEMPERATURE,
    NOVA_DIM_AMOUNT,
    NOVA_DIM_LUMINOSITY,
} nova_base_dimension_t;

/* Create dimensions */
nova_dimension_t *nova_dim_dimensionless(void);
nova_dimension_t *nova_dim_base(nova_base_dimension_t base);

/* Base SI units */
nova_dimension_t *nova_dim_meter(void);
nova_dimension_t *nova_dim_kilogram(void);
nova_dimension_t *nova_dim_second(void);
nova_dimension_t *nova_dim_ampere(void);
nova_dimension_t *nova_dim_kelvin(void);

/* Derived SI units */
nova_dimension_t *nova_dim_newton(void);    /* Force: kg⋅m⋅s⁻² */
nova_dimension_t *nova_dim_joule(void);     /* Energy: kg⋅m²⋅s⁻² */
nova_dimension_t *nova_dim_watt(void);      /* Power: kg⋅m²⋅s⁻³ */
nova_dimension_t *nova_dim_volt(void);      /* Voltage: kg⋅m²⋅s⁻³⋅A⁻¹ */

/* ── Extended SI derived units ──────────────────────────────────────────── */
nova_dimension_t *nova_dim_pascal(void);    /* Pressure:     Pa  = kg⋅m⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_hertz(void);     /* Frequency:    Hz  = s⁻¹ */
nova_dimension_t *nova_dim_coulomb(void);   /* Charge:       C   = A⋅s */
nova_dimension_t *nova_dim_ohm(void);       /* Resistance:   Ω   = kg⋅m²⋅A⁻²⋅s⁻³ */
nova_dimension_t *nova_dim_farad(void);     /* Capacitance:  F   = kg⁻¹⋅m⁻²⋅A²⋅s⁴ */
nova_dimension_t *nova_dim_tesla(void);     /* Flux density: T   = kg⋅A⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_henry(void);     /* Inductance:   H   = kg⋅m²⋅A⁻²⋅s⁻² */
nova_dimension_t *nova_dim_weber(void);     /* Magnetic flux:Wb  = kg⋅m²⋅A⁻¹⋅s⁻² */
nova_dimension_t *nova_dim_siemens(void);   /* Conductance:  S   = kg⁻¹⋅m⁻²⋅A²⋅s³ */
nova_dimension_t *nova_dim_lumen(void);     /* Lum. flux:    lm  = cd */
nova_dimension_t *nova_dim_lux(void);       /* Illuminance:  lx  = cd⋅m⁻² */
nova_dimension_t *nova_dim_becquerel(void); /* Radioactivity:Bq  = s⁻¹ (= Hz dim) */
nova_dimension_t *nova_dim_gray(void);      /* Absorbed dose:Gy  = m²⋅s⁻² */
nova_dimension_t *nova_dim_sievert(void);   /* Eff. dose:    Sv  = m²⋅s⁻² */
nova_dimension_t *nova_dim_katal(void);     /* Catalytic:    kat = mol⋅s⁻¹ */

/* Dimension operations */
nova_dimension_t *nova_dim_multiply(const nova_dimension_t *a, const nova_dimension_t *b);
nova_dimension_t *nova_dim_divide(const nova_dimension_t *a, const nova_dimension_t *b);
nova_dimension_t *nova_dim_power(const nova_dimension_t *dim, int exponent);

/* Dimension checking */
bool nova_dim_compatible(const nova_dimension_t *a, const nova_dimension_t *b);
bool nova_dim_is_dimensionless(const nova_dimension_t *dim);

/* String representation */
const char *nova_dim_to_string(const nova_dimension_t *dim);
nova_dimension_t *nova_dim_parse(const char *str);

/* Unit conversion */
nova_dimension_t *nova_dim_with_scale(const nova_dimension_t *base, double scale);
double nova_dim_convert(double value, const nova_dimension_t *from, const nova_dimension_t *to);

/* Cleanup */
void nova_dim_destroy(nova_dimension_t *dim);

/// Get the scale factor of a dimension (e.g. km → 1000.0, m → 1.0)
double nova_dim_get_scale(const nova_dimension_t *dim);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_DIMENSIONS_H */
