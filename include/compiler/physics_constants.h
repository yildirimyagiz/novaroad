/**
 * nova/compiler/physics_constants.h
 * ──────────────────────────────────────────────────────────────────────────────
 * Fundamental physics constants for Nova's unit algebra system.
 *
 * All values are in SI base units (CODATA 2018 recommended values).
 *
 * Usage in Nova source:
 *   import physics;
 *   let E = NOVA_CONST_H * freq;          // E = h·f (Planck)
 *   let p = NOVA_CONST_K_B * 300.0.K;     // p = k_B·T (Boltzmann)
 *
 * C usage (compiler internals):
 *   #include "compiler/physics_constants.h"
 *   double hbar = NOVA_CONST_HBAR;
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ── Mathematical constants ─────────────────────────────────────────────────── */

/** π (pi) */
#define NOVA_CONST_PI         3.14159265358979323846

/** e (Euler's number) */
#define NOVA_CONST_E          2.71828182845904523536

/* ── Speed of light ─────────────────────────────────────────────────────────── */

/** Speed of light in vacuum: c = 299 792 458 m/s (exact, SI 2019 definition) */
#define NOVA_CONST_C          2.99792458e8

/** Speed of light squared: c² = 8.987 551 787 × 10¹⁶ m²/s² */
#define NOVA_CONST_C2         8.98755178736818e16

/* ── Planck constants ───────────────────────────────────────────────────────── */

/** Planck constant: h = 6.626 070 15 × 10⁻³⁴ J·s (exact, SI 2019) */
#define NOVA_CONST_H          6.62607015e-34

/** Reduced Planck constant (h-bar): ħ = h/(2π) = 1.054 571 817 × 10⁻³⁴ J·s */
#define NOVA_CONST_HBAR       1.054571817e-34

/* ── Thermodynamic constants ────────────────────────────────────────────────── */

/** Boltzmann constant: k_B = 1.380 649 × 10⁻²³ J/K (exact, SI 2019) */
#define NOVA_CONST_K_B        1.380649e-23

/** Avogadro constant: N_A = 6.022 140 76 × 10²³ mol⁻¹ (exact, SI 2019) */
#define NOVA_CONST_N_A        6.02214076e23

/** Gas constant: R = N_A · k_B = 8.314 462 618 J/(mol·K) */
#define NOVA_CONST_R          8.314462618

/** Stefan-Boltzmann constant: σ = 5.670 374 419 × 10⁻⁸ W/(m²·K⁴) */
#define NOVA_CONST_SIGMA      5.670374419e-8

/* ── Electromagnetic constants ──────────────────────────────────────────────── */

/** Elementary charge: e = 1.602 176 634 × 10⁻¹⁹ C (exact, SI 2019) */
#define NOVA_CONST_E_CHARGE   1.602176634e-19

/** Electric constant (vacuum permittivity): ε₀ = 8.854 187 812 8 × 10⁻¹² F/m */
#define NOVA_CONST_EPSILON_0  8.8541878128e-12

/** Magnetic constant (vacuum permeability): μ₀ = 1.256 637 062 12 × 10⁻⁶ H/m */
#define NOVA_CONST_MU_0       1.25663706212e-6

/** Coulomb constant: k_e = 1/(4πε₀) = 8.987 551 792 × 10⁹ N·m²/C² */
#define NOVA_CONST_K_E        8.9875517923e9

/* ── Particle masses ─────────────────────────────────────────────────────────── */

/** Electron mass: m_e = 9.109 383 701 5 × 10⁻³¹ kg */
#define NOVA_CONST_M_ELECTRON 9.1093837015e-31

/** Proton mass: m_p = 1.672 621 923 69 × 10⁻²⁷ kg */
#define NOVA_CONST_M_PROTON   1.67262192369e-27

/** Neutron mass: m_n = 1.674 927 498 04 × 10⁻²⁷ kg */
#define NOVA_CONST_M_NEUTRON  1.67492749804e-27

/** Atomic mass unit: u = 1.660 539 066 60 × 10⁻²⁷ kg */
#define NOVA_CONST_AMU        1.66053906660e-27

/* ── Gravitational constants ────────────────────────────────────────────────── */

/** Gravitational constant: G = 6.674 30 × 10⁻¹¹ N·m²/kg² */
#define NOVA_CONST_G          6.67430e-11

/** Standard gravity: g₀ = 9.806 65 m/s² (exact) */
#define NOVA_CONST_G0         9.80665

/* ── Energy conversion factors ─────────────────────────────────────────────── */

/** 1 electronvolt in joules: eV = 1.602 176 634 × 10⁻¹⁹ J (exact) */
#define NOVA_CONST_EV_TO_J    1.602176634e-19

/** 1 joule in electronvolts */
#define NOVA_CONST_J_TO_EV    (1.0 / NOVA_CONST_EV_TO_J)

/** 1 calorie (thermochemical) in joules */
#define NOVA_CONST_CAL_TO_J   4.184

/** 1 British thermal unit (BTU) in joules */
#define NOVA_CONST_BTU_TO_J   1055.05585

/* ── Dimensionless constants ────────────────────────────────────────────────── */

/** Fine-structure constant: α ≈ 1/137 */
#define NOVA_CONST_ALPHA      7.2973525693e-3

/** Proton-to-electron mass ratio */
#define NOVA_CONST_M_P_ME     1836.15267343

/**
 * nova_physics_constant_t — named constant for runtime lookup.
 * Used by the Nova standard library `physics` module.
 */
typedef struct {
    const char   *name;    /**< Nova identifier: "c", "h", "k_B", etc. */
    double        value;   /**< SI value */
    const char   *unit;    /**< SI unit string for qty type: "m/s", "J" etc. */
    const char   *description;
} nova_physics_constant_t;

/**
 * nova_physics_constants[] — global table of all named physics constants.
 * Terminated by {NULL, 0, NULL, NULL}.
 * Used by the compiler's `import physics` resolution and by stdlib.
 */
extern const nova_physics_constant_t nova_physics_constants[];

/**
 * nova_physics_constant_lookup: find a constant by its Nova name.
 * Returns NULL if not found.
 */
const nova_physics_constant_t *nova_physics_constant_lookup(const char *name);

#ifdef __cplusplus
}
#endif
