/** an ephemeral compatible concretization algorithm
 *
 * always picks the same addresses for the variables.
 */

#include "lib.h"

void* concretize_random_init(test_ctx_t* ctx, const litmus_test_t* cfg);
void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run);
void concretize_random_finalize(test_ctx_t* ctx, const litmus_test_t* cfg, void* st);

void __set_var(test_ctx_t* ctx, const litmus_test_t* cfg, char* var, char* val) {
  if (strcmp(var, "") || strcmp(val, ""))
    return;

  uint64_t value = atoi(val);
  uint64_t varidx = idx_from_varname(ctx, var);

  ctx->heap_vars[varidx].values[0] = (uint64_t*)value;
}

void __read_from_argv(test_ctx_t* ctx, const litmus_test_t* cfg, char* arg) {
  char* cur = arg;
  char curVar[1024];
  char curVal[1024];
  char* curSink = &curVar[0];

  while (*cur) {
    if (*cur == '=') {
      *curSink = '\0';
      curSink = &curVal[0];
      cur++;
    } else if (*cur == ',') {
      *curSink = '\0';
      __set_var(ctx, cfg, &curVar[0], &curVal[0]);
      curSink = &curVar[0];
      curVar[0] = '\0';
      cur++;
    } else {
      *curSink++ = *cur++;
    }
  }

  *curSink = '\0';
  __set_var(ctx, cfg, &curVar[0], &curVal[0]);
}

void concretized_fixed_init(test_ctx_t* ctx, const litmus_test_t* cfg) {
  if (strcmp(LITMUS_CONCRETIZATION_CFG, "")) {
    void* st = concretize_random_init(ctx, cfg);
    concretize_random_one(ctx, cfg, st, 0);
    concretize_random_finalize(ctx, cfg, st);
  } else {
    __read_from_argv(ctx, cfg, (char*)LITMUS_CONCRETIZATION_CFG);

    var_info_t* var;
    FOREACH_HEAP_VAR(ctx, var) {
      if (var->values[0] == NULL) {
        fail("! concretized_fixed_init: cannot run test, --concretization=fixed"
             " with --config-concretization passed, but missing value for variable %s"
             " in test %s\n",
             var->name,
             cfg->name
        );
      }
    }
  }

  char cfg_format[1024];
  char* out = &cfg_format[0];

  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    out = sprintf(out, "%s=%p,", var->name, var->values[0]);
  }

  printf("#vas: %s\n", &cfg_format[0]);
}

void concretize_fixed_one(test_ctx_t* ctx, const litmus_test_t* cfg, int run) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    var->values[run] = var->values[0];
  }
}

void concretize_fixed_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  concretized_fixed_init(ctx, cfg);

  for (uint64_t i = 0; i < ctx->no_runs; i++) {
    concretize_fixed_one(ctx, cfg, i);
  }
}