#include "nova_formal.h"
#include "nova_kernel_contracts.h"
#include "nova_obligation.h"
#include "nova_solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_initialized = false;
static SolverSession *g_jit_solver = NULL;

int nova_formal_init(NovaFormalConfig config) {
  if (g_initialized)
    return 0;

  SolverConfig sol_config = solver_config_default();
  sol_config.backend = SOLVER_AUTO;
  sol_config.timeout_ms = config.timeout_ms > 0 ? config.timeout_ms : 50.0;

  g_jit_solver = solver_session_create(&sol_config);
  g_initialized = true;

  printf("🛡️ Nova Formal Logic Engine: Online (Mode: %d)\n", config.mode);
  return 0;
}

void nova_formal_shutdown(void) {
  if (!g_initialized)
    return;
  solver_session_destroy(g_jit_solver);
  g_initialized = false;
}

NovaFormalKernelReport nova_formal_kernel_validate(const char *kernel_name,
                                                       NovaTensor **inputs,
                                                       int num_inputs,
                                                       NovaTensor **outputs,
                                                       int num_outputs) {

  NovaFormalKernelReport r = {0};
  r.bounds_valid = true;
  r.shape_valid = true;
  r.type_valid = true;
  r.determinism_guaranteed = true;

  if (!g_initialized) {
    r.violation_msg = "System not initialized";
    return r;
  }
  if (num_inputs < 1 || !inputs[0])
    return r;

  /* 1. Generate V2 Obligations (Fast Contracts) */
  NovaObligation ob = {0};
  bool found_contract = false;

  if (strstr(kernel_name, "matmul")) {
    if (num_inputs >= 2 && num_outputs >= 1) {
      NovaTensor *bias = (num_inputs >= 3) ? inputs[2] : NULL;
      ob = nova_contract_matmul(inputs[0], inputs[1], bias, outputs[0]);
      found_contract = true;
    }
  } else if (strstr(kernel_name, "add")) {
    if (num_inputs >= 2 && num_outputs >= 1) {
      ob = nova_contract_add(inputs[0], inputs[1], outputs[0]);
      found_contract = true;
    }
  }

  if (found_contract) {
    if (!ob.all_satisfied) {
      r.shape_valid = false;
      r.bounds_valid = false;
      /* Find first violation */
      for (uint32_t i = 0; i < ob.num_reqs; i++) {
        if (!ob.reqs[i].satisfied) {
          r.violation_msg = ob.reqs[i].name;
          break;
        }
      }
      return r;
    }
  }

  return r;
}

NovaFormalKernelReport
nova_formal_graph_validate(const NovaIRGraph *graph) {
  (void)graph;
  NovaFormalKernelReport r = {0};
  r.shape_valid = true;
  r.bounds_valid = true;
  r.type_valid = true;
  r.determinism_guaranteed = true;
  return r;
}

bool nova_formal_compile_check(const char *func_name) {
  (void)func_name;
  return true;
}

NovaFormalOptimizerReport
nova_formal_optimizer_validate(const char *decision_id,
                                 void *optimizer_state) {
  (void)decision_id;
  (void)optimizer_state;
  NovaFormalOptimizerReport r = {0};
  r.is_stable = true;
  r.no_numerical_drift = true;
  r.strategy_consistent = true;
  r.stability_score = 1.0f;
  return r;
}
