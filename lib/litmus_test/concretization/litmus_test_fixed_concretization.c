/** an ephemeral compatible concretization algorithm
 *
 * always picks the same addresses for the variables.
 */

#include "lib.h"

void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, int run);

void concretize_fixed_one(test_ctx_t* ctx, const litmus_test_t* cfg, int run) {
  static uint8_t picked_vars = 0;
  if (picked_vars == 0) {
    concretize_random_one(ctx, cfg, 0);
    picked_vars = 1;
  }

  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    var->values[run] = var->values[0];
  }
}

void concretize_fixed_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  for (uint64_t i = 0; i < ctx->no_runs; i++) {
    concretize_fixed_one(ctx, cfg, i);
  }
}