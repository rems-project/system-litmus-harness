#include "lib.h"

static void fail_postcheck(test_ctx_t* ctx, const litmus_test_t* cfg, run_idx_t run, const char* fmt, ...) {
  char* buf = ALLOC_MANY(char, 1024);
  va_list ap;
  va_start(ap, fmt);
  vsprintf(NEW_BUFFER(buf, 1024), 1, fmt, ap);
  va_end(ap);
  fail("! error: concretization of test %s failed on run %d: %s\n", cfg->name, run, buf);
}

/** check for nonsensical (bad) configurations */
void concretization_precheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    fail_on(var->ty == VAR_NOTSET, "cannot concretize a NOTSET variable\n");
  }
}

/* if var A owns a region R, and var B is not pinned to A but is also in region R
 * then fail
 */
static void
concretize_postcheck_no_overlap_owned(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_idx_t run) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    if (var->ty == VAR_HEAP) {
      var_info_t* othervar;
      FOREACH_HEAP_VAR(ctx, othervar) {
        if (var->varidx == othervar->varidx)
          continue;

        u64 va1 = (u64)var->values[run];
        u64 va2 = (u64)othervar->values[run];
        if (in_same_region(va1, va2, var->heap.owned_region_size)
            && (   othervar->ty != VAR_PINNED
                || othervar->pin.pin_region_var != var->varidx)
        ) {
          fail_postcheck(
            ctx,
            cfg,
            run,
            "%s (%p) and %s (%p) overlap but one is not pinned to the other\n",
            var->name,
            va1,
            othervar->name,
            va2
          );
        }
      }
    }
  }
}

/* if var A states an offset from var B,  check that the last N bits of the VAs actually match!
 */
static void
concretize_postcheck_related_same_bits(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_idx_t run) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    if (var->init_attrs.has_region_offset) {
      u64 offsvaridx = var->init_attrs.region_offset.offset_var;
      var_info_t* othervar = &ctx->heap_vars[offsvaridx];

      u64 va1 = (u64)var->values[run];
      u64 va2 = (u64)othervar->values[run];

      u64 lvl = LEVEL_SHIFTS[var->init_attrs.region_offset.offset_level];

      if ((va1 & BITMASK(lvl)) != (va2 & BITMASK(lvl))) {
        fail_postcheck(
          ctx,
          cfg,
          run,
          "%s (%p) and %s (%p) are supposed to have the same %s but do not\n",
          var->name,
          va1,
          othervar->name,
          va2,
          rel_offset_to_str(var->init_attrs.region_offset.offset_level)
        );
      }
    }
  }
}

/* if var A states an offset from var B,  check that the last N bits of the VAs actually match!
 */
static void concretize_postcheck_aligned(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_idx_t run) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    u64 va = (u64)var->values[run];

    if ((va & BITMASK(3)) != 0) {
      fail_postcheck(ctx, cfg, run, "%s (%p) is supposed to be 64-bit aligned but is not.\n", var->name, va);
    }
  }
}

/* post checker
 *
 * this runs _after_ concretization to make sure it's valid
 */
void concretization_postcheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_idx_t run) {
  concretize_postcheck_no_overlap_owned(ctx, cfg, infos, run);
  concretize_postcheck_related_same_bits(ctx, cfg, infos, run);
  concretize_postcheck_aligned(ctx, cfg, infos, run);
}

/* if one run allocates some PA, the others in the same patch should not.
 */
static void
__concretize_postcheck_batch_var_overlaps_pa(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_count_t batch_start_idx, run_count_t this, run_count_t batch_end_idx, var_info_t* var) {
  var_info_t* other_var;
  run_idx_t this_idx = count_to_run_index(ctx, this);
  u64 this_pa = backed_phys_addr(var, this_idx);

  for (run_count_t other = batch_start_idx; other < this; other++) {
    run_idx_t other_idx = count_to_run_index(ctx, other);

    if (other == this)
      continue;

    FOREACH_HEAP_VAR(ctx, other_var) {
      if (! is_backed_var(other_var))
        continue;

      u64 other_pa = backed_phys_addr(other_var, other_idx);

      if (this_pa == other_pa)
        fail_postcheck(ctx, cfg, (this < other ? this : other), "overlapping PAs in same batch (var '%s' in run %ld and '%s' in run %ld)", var->name, this, other_var->name, other);
    }
  }
}

static void
concretize_postcheck_batch_overlaps_pa(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_count_t batch_start_idx, run_count_t this, run_count_t batch_end_idx) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    if (is_backed_var(var))
      __concretize_postcheck_batch_var_overlaps_pa(ctx, cfg, infos, batch_start_idx, this, batch_end_idx, var);
  }
}

/* post checker for batched ASID runs
 *
 * for batched runs with ASIDs, it's okay if there overlap between the runs in the VAs
 * but not for the PAs.
 */
 void concretization_postcheck_batch(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, run_count_t batch_start_idx, run_count_t batch_end_idx) {
  for (run_count_t r = batch_start_idx; r < batch_end_idx; r++) {
    run_idx_t run = count_to_run_index(ctx, r);
    concretization_postcheck(ctx, cfg, infos, run);
    concretize_postcheck_batch_overlaps_pa(ctx, cfg, infos, batch_start_idx, r, batch_end_idx);
  }
}