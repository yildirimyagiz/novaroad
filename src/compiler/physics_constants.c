#include <stdbool.h>

/**
 * src/compiler/physics_constants.c
 * ──────────────────────────────────────────────────────────────────────────────
 * Runtime table of fundamental physics constants for Nova's unit algebra system.
 * All values CODATA 2018 / SI 2019.
 */

#include "compiler/physics_constants.h"
#include <string.h>

/* ── Global constant table ──────────────────────────────────────────────────── */
const nova_physics_constant_t nova_physics_constants[] = {
    /* Speed of light */
    { "c",          NOVA_CONST_C,          "m/s",     "Speed of light in vacuum" },
    { "c2",         NOVA_CONST_C2,         "m2/s2",   "Speed of light squared" },

    /* Planck */
    { "h",          NOVA_CONST_H,          "J*s",     "Planck constant" },
    { "hbar",       NOVA_CONST_HBAR,       "J*s",     "Reduced Planck constant (h/2pi)" },

    /* Thermodynamic */
    { "k_B",        NOVA_CONST_K_B,        "J/K",     "Boltzmann constant" },
    { "k_b",        NOVA_CONST_K_B,        "J/K",     "Boltzmann constant (lowercase)" },
    { "N_A",        NOVA_CONST_N_A,        "mol",     "Avogadro constant" },
    { "R",          NOVA_CONST_R,          "J/mol/K", "Molar gas constant" },
    { "sigma",      NOVA_CONST_SIGMA,      "W/m2/K4", "Stefan-Boltzmann constant" },

    /* Electromagnetic */
    { "e",          NOVA_CONST_E_CHARGE,   "C",       "Elementary charge" },
    { "eps0",       NOVA_CONST_EPSILON_0,  "F/m",     "Electric permittivity of vacuum" },
    { "epsilon_0",  NOVA_CONST_EPSILON_0,  "F/m",     "Electric permittivity of vacuum" },
    { "mu0",        NOVA_CONST_MU_0,       "H/m",     "Magnetic permeability of vacuum" },
    { "mu_0",       NOVA_CONST_MU_0,       "H/m",     "Magnetic permeability of vacuum" },
    { "k_e",        NOVA_CONST_K_E,        "N*m2/C2", "Coulomb constant" },

    /* Particle masses */
    { "m_e",        NOVA_CONST_M_ELECTRON, "kg",      "Electron mass" },
    { "m_p",        NOVA_CONST_M_PROTON,   "kg",      "Proton mass" },
    { "m_n",        NOVA_CONST_M_NEUTRON,  "kg",      "Neutron mass" },
    { "u",          NOVA_CONST_AMU,        "kg",      "Atomic mass unit" },
    { "amu",        NOVA_CONST_AMU,        "kg",      "Atomic mass unit" },

    /* Gravity */
    { "G",          NOVA_CONST_G,          "N*m2/kg2","Gravitational constant" },
    { "g0",         NOVA_CONST_G0,         "m/s2",    "Standard gravity acceleration" },
    { "g_n",        NOVA_CONST_G0,         "m/s2",    "Standard gravity acceleration" },

    /* Energy conversion */
    { "eV",         NOVA_CONST_EV_TO_J,    "J",       "Electronvolt in joules" },

    /* Dimensionless */
    { "alpha",      NOVA_CONST_ALPHA,      "",        "Fine-structure constant" },
    { "pi",         NOVA_CONST_PI,         "",        "Mathematical pi" },
    { "euler_e",    NOVA_CONST_E,          "",        "Euler's number" },

    /* Sentinel */
    { NULL, 0.0, NULL, NULL }
};

/* ── Lookup ──────────────────────────────────────────────────────────────────── */
const nova_physics_constant_t *nova_physics_constant_lookup(const char *name)
{
    if (!name) return NULL;
    for (const nova_physics_constant_t *c = nova_physics_constants; c->name; c++) {
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}
