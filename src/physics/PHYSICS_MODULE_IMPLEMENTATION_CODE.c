/**
 * @file PHYSICS_MODULE_IMPLEMENTATION_CODE.c
 * 
 * Reference implementation code for adding physics module support to Nova compiler.
 * Insert this into src/compiler/semantic.c
 * 
 * Location: After line 1340, before the default case in analyze_statement_with_context()
 */

// ══════════════════════════════════════════════════════════════════════════════
// PHYSICS MODULE SUPPORT (NEW)
// ══════════════════════════════════════════════════════════════════════════════

/**
 * register_physics_constant: Helper to register a single physics constant
 * 
 * @param table Symbol table to register into
 * @param name Constant name (e.g., "c", "h")
 * @param unit_expr Unit expression string (e.g., "m/s", "J*s")
 */
static void register_physics_constant(symbol_table_t *table, const char *name,
                                      const char *unit_expr)
{
    nova_type_t *qty_type = nova_type_qty(nova_type_f64(), unit_expr);
    symbol_table_add(table, name, qty_type, false);  /* immutable constant */
    nova_type_free(qty_type);  /* symbol_table_add clones, so free original */
}

/**
 * handle_import_statement: Process import statements
 * 
 * Handles module loading for `import physics;`, `import physics::{c, h};`, etc.
 * Currently supports:
 *   - Wildcard: import physics::*;
 *   - Selective: import physics::{c, h, k_B};
 *   - Simple: import physics;
 * 
 * @return true if import is valid, false if error
 */
static bool handle_import_statement(nova_semantic_t *semantic, nova_stmt_t *stmt,
                                    symbol_table_t *table)
{
    if (!stmt || stmt->kind != STMT_IMPORT)
        return false;

    const char *module = stmt->data.import_stmt.module_name;
    const char **imports = (const char **)stmt->data.import_stmt.imports;
    size_t import_count = stmt->data.import_stmt.import_count;

    /* Physics module support */
    if (strcmp(module, "physics") == 0) {
        /* Determine if wildcard or selective import */
        bool is_wildcard = (import_count == 0 || imports == NULL);

        if (is_wildcard) {
            /* Wildcard import: register ALL physics constants */
            
            /* Fundamental Constants */
            register_physics_constant(table, "c", "m/s");              /* Speed of light */
            register_physics_constant(table, "h", "J*s");              /* Planck constant */
            register_physics_constant(table, "hbar", "J*s");           /* Reduced Planck */
            register_physics_constant(table, "k_B", "J/K");            /* Boltzmann constant */
            register_physics_constant(table, "G", "m^3/(kg*s^2)");     /* Gravitational constant */
            register_physics_constant(table, "e", "C");                /* Elementary charge */
            register_physics_constant(table, "N_A", "1/mol");          /* Avogadro constant */
            
            /* Additional useful constants */
            register_physics_constant(table, "epsilon_0", "F/m");      /* Permittivity */
            register_physics_constant(table, "mu_0", "H/m");           /* Permeability */
            register_physics_constant(table, "sigma", "W/(m^2*K^4)");  /* Stefan-Boltzmann */
            register_physics_constant(table, "alpha", "");             /* Fine structure (dimensionless) */
            
        } else {
            /* Selective import: register only named constants */
            for (size_t i = 0; i < import_count; i++) {
                const char *name = imports[i];
                
                /* Map constant name to unit expression */
                if (strcmp(name, "c") == 0) {
                    register_physics_constant(table, "c", "m/s");
                }
                else if (strcmp(name, "h") == 0) {
                    register_physics_constant(table, "h", "J*s");
                }
                else if (strcmp(name, "hbar") == 0) {
                    register_physics_constant(table, "hbar", "J*s");
                }
                else if (strcmp(name, "k_B") == 0) {
                    register_physics_constant(table, "k_B", "J/K");
                }
                else if (strcmp(name, "G") == 0) {
                    register_physics_constant(table, "G", "m^3/(kg*s^2)");
                }
                else if (strcmp(name, "e") == 0) {
                    register_physics_constant(table, "e", "C");
                }
                else if (strcmp(name, "N_A") == 0) {
                    register_physics_constant(table, "N_A", "1/mol");
                }
                else if (strcmp(name, "epsilon_0") == 0) {
                    register_physics_constant(table, "epsilon_0", "F/m");
                }
                else if (strcmp(name, "mu_0") == 0) {
                    register_physics_constant(table, "mu_0", "H/m");
                }
                else if (strcmp(name, "sigma") == 0) {
                    register_physics_constant(table, "sigma", "W/(m^2*K^4)");
                }
                else if (strcmp(name, "alpha") == 0) {
                    register_physics_constant(table, "alpha", "");
                }
                else {
                    /* Unknown physics constant */
                    semantic_set_error(semantic, 
                        "Unknown physics constant '%s'", name);
                    return false;
                }
            }
        }

        return true;
    }

    /* Other modules can be added here */
    
    /* Unknown module (non-fatal for now) */
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// INTEGRATION POINT (in analyze_statement_with_context)
// ══════════════════════════════════════════════════════════════════════════════

/*
INSERT THIS CASE BLOCK IN analyze_statement_with_context() AROUND LINE 1330:

    case STMT_IMPORT: {
        return handle_import_statement(semantic, stmt, table);
    }

FULL SWITCH STATEMENT CONTEXT (lines 996-1340+):

    switch (stmt->kind) {
    case STMT_VAR_DECL: {
        // ... existing code ...
    }
    
    case STMT_EXPR: {
        // ... existing code ...
    }
    
    // ... other cases ...
    
    case STMT_STRUCT_DECL: {
        // ... existing code ...
    }
    
    case STMT_IMPORT: {                          // ← ADD THIS
        return handle_import_statement(semantic, stmt, table);
    }
    
    default:
        // ... existing default handling ...
        return false;
    }
*/

// ══════════════════════════════════════════════════════════════════════════════
// TEST CODE (Nova language)
// ══════════════════════════════════════════════════════════════════════════════

/*
After implementation, test with:

File: test_physics_import.nova
─────────────────────────────

import physics;

fn main() {
    // Test 1: Wildcard import access
    let speed: qty<f64, m/s> = c;
    println(speed);  // Should print the type and value
    
    // Test 2: Type checking with qty
    let planck: qty<f64, J*s> = h;
    
    // Test 3: Unit conversion
    let c_in_km_s = c in km/s;
    println(c_in_km_s);
}

File: test_physics_selective.nova
──────────────────────────────────

import physics::{c, h, k_B};

fn main() {
    // Only c, h, k_B are in scope
    let speed = c;
    let planck = h;
    let boltzmann = k_B;
    
    // This would fail to compile:
    // let grav = G;  // ERROR: Undefined variable 'G'
}

File: test_physics_alias.nova
──────────────────────────────

import physics as phys;

fn main() {
    // Access with alias: phys::c
    // (requires additional implementation for namespaced access)
}
*/

// ══════════════════════════════════════════════════════════════════════════════
// DIMENSIONAL ANALYSIS REFERENCE
// ══════════════════════════════════════════════════════════════════════════════

/*
Unit expressions in nova_type_qty():

Fundamental SI units:
  m       - meter (length)
  kg      - kilogram (mass)
  s       - second (time)
  A       - ampere (current)
  K       - kelvin (temperature)
  mol     - mole (amount)
  cd      - candela (luminance)

Derived units (can be expressed as combinations):
  J       - joule = kg*m^2/s^2
  W       - watt = kg*m^2/s^3 = J/s
  C       - coulomb = A*s
  V       - volt = kg*m^2/(A*s^3) = W/A
  F       - farad = A^2*s^4/(kg*m^2) = C/V
  H       - henry = kg*m^2/(A^2*s^2) = V*s/A

Examples:
  "m/s"           - velocity
  "m/s^2"         - acceleration
  "kg*m/s^2"      - force (newton)
  "J*s"           - action (h, hbar)
  "J/K"           - entropy (k_B)
  "m^3/(kg*s^2)"  - gravitational constant
  "C"             - charge
  "1/mol"         - Avogadro (dimensionless per substance)
  "F/m"           - permittivity
  "H/m"           - permeability
  "W/(m^2*K^4)"   - Stefan-Boltzmann constant
*/

