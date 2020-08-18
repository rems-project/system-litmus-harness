#include "lib.h"

/** place each VA in its own page,
 * starting with the first VA in page#1 then filling the next N pages
 * until getting to the second VA, etc
 */

void* concretize_linear_alloc_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  uint64_t* p = alloc(sizeof(uint64_t*));
  uint64_t* start = (uint64_t* )ctx->heap_memory;

  *p = start;
  return (void*)p;
}

void concretize_linear_one(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run) {
  uint64_t* p = st;
  *p = (uint64_t* )ALIGN_UP_TO((uint64_t)*p, 4096L);

  var_info_t* vinfo = &ctx->heap_vars[v];

  for (int v = 0; v < cfg->no_heap_vars; v++) {
    vinfo->values[run] = *p;
    *p++;
  }
}

void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int no_runs) {
  for (uint64_t i = 0; i < ctx->no_runs; i++) {
    concretize_linear_one(ctx, cfg, st, i);
  }
}

void concretize_linear_free_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st) {
  free(st);
}
