#include "lib.h"

/** place each VA in its own page,
 * starting with the first VA in page#1 then filling the next N pages
 * until getting to the second VA, etc
 */
void concretize_linear(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  uint64_t* start = (uint64_t* )ctx->heap_memory;

  uint64_t* p = start;
  for (int v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &ctx->heap_vars[v];

    for (uint64_t i = 0; i < no_runs; i++) {
      vinfo->values[i] = p;
      p++;
    }

    p = (uint64_t* )ALIGN_UP_TO((uint64_t)p, 4096L);
  }


  for (uint64_t i = 0; i < no_runs; i++) {
    for (int v = 0; v < cfg->no_heap_vars; v++) {
      set_init_var(ctx, infos, v, i);
    }
  }
}