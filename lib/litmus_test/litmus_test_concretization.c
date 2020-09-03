#include "lib.h"

/* random */
extern void* concretize_random_alloc_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
extern void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run);
extern void concretize_random_all(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int no_runs);
extern void concretize_random_free_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st);

/* linear */
extern void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);

/** given a var and an index perform the necessary initialization
 * this is unsynchronized so TLB maintenance must be performed after the fact.
 *
 * assumes all other vars have had their values[idx] set
 */
void set_init_var(test_ctx_t* ctx, var_info_t* infos, uint64_t varidx, uint64_t idx) {
  var_info_t* vinfo = &infos[varidx];

  uint64_t* va = vinfo->values[idx];
  *va = vinfo->init_value;
  uint64_t* pte = ctx_pte(ctx, (uint64_t)va);

  if (vinfo->init_unmapped != 0) {
    *pte = 0;
  }

  if (vinfo->init_ap != 0) {
    desc_t desc = read_desc(*pte, 3);
    desc.attrs.AP = vinfo->init_ap;
    *pte = write_desc(desc);
  }

  /* this means we must ensure that all VAs for this idx are chosen
   * before attempting to set_init_var
   */
  if (vinfo->alias) {
    uint64_t otheridx = idx_from_varname(ctx, vinfo->alias);
    uint64_t otherva = (uint64_t )ctx->heap_vars[otheridx].values[idx];

    /* do not copy attrs of otherpte */
    desc_t desc = read_desc(*pte, 3);
    desc.oa = PAGE(otherva);
    *pte = write_desc(desc);
  }
}

void init_vars(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  for (int v = 0; v < cfg->no_heap_vars; v++) {
    set_init_var(ctx, infos, v, run);
  }

  /* check that the concretization was successful before continuing */
  concretization_postcheck(ctx, cfg, infos, run);
}

void pick_concrete_addrs(test_ctx_t* ctx, const litmus_test_t* cfg, int run);

void concretize_one(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run) {
  switch (type) {
    case CONCRETE_LINEAR:
      fail("! concretize: cannot concretize_one with concretization=linear\n");
      break;
    case CONCRETE_RANDOM:
      concretize_random_one(ctx, cfg, st, run);
      break;
    default:
      fail("! concretize: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }

  init_vars(ctx, cfg, ctx->heap_vars, run);
}

void concretize(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  /* check that the test _can_ be concretized correctly
   * or fail if there's some error */
  concretization_precheck(ctx, cfg, infos);

  switch (type) {
    case CONCRETE_LINEAR:
      concretize_linear_all(ctx, cfg, no_runs);
      break;
    case CONCRETE_RANDOM:
      concretize_random_all(ctx, cfg, infos, no_runs);
    default:
      fail("! concretize: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }

  /* ensure all vars have separate level 3 entries before continuing
   * otherwise an access in set_init_var could cause a vmm_ensure_level to happen
   * which would copy a previous var's pte attributes.
   */
  for (uint64_t i = 0; i < no_runs; i++) {
    for (int v = 0; v < cfg->no_heap_vars; v++) {
      vmm_ensure_level(ctx->ptable, 3, (uint64_t)infos[v].values[i]);
    }
  }

  for (uint64_t i = 0; i < no_runs; i++) {
    init_vars(ctx, cfg, ctx->heap_vars, i);
  }
}

void* concretize_allocate_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  switch (type) {
    case CONCRETE_LINEAR:
      break;
    case CONCRETE_RANDOM:
      break;
    default:
      fail("! concretize_allocate_st: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }

  return NULL;
}

void concretize_free_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st) {
  switch (type) {
    case CONCRETE_LINEAR:
      break;
    case CONCRETE_RANDOM:
      break;
    default:
      fail("! concretize_free_st: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }
}