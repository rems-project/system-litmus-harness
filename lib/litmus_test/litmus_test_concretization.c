#include "lib.h"

extern void concretize_all_linear(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
extern void concretize_one_linear(test_ctx_t* ctx, const litmus_test_t* cfg, int run);
extern void concretize_standard(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);

concretization_algo_t LINEAR_CONCRETIZATION_ALGO = {
  .concretize_all = concretize_all_linear,
  .concretize_one = concretize_one_linear,
}
concretization_algo_t STANDARD_CONCRETIZATION_ALGO;

extern concretization_algo_t* CONCRETIZATION_ALGO;

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

  printf("var %s @ %p/%p  (desc %p)\n", vinfo->name, va, pte, *pte);
  if (vinfo->init_unmapped != 0) {
    printf("unmapping %s \n", vinfo->name);
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

void pick_concrete_addrs(test_ctx_t* ctx, const litmus_test_t* cfg, int run);

void concretize(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  /* check that the test _can_ be concretized correctly
   * or fail if there's some error */
  concretization_precheck(ctx, cfg, infos);

  switch (LITMUS_CONCRETIZATION_TYPE) {
    case CONCRETE_STANDARD:
      concretize_standard(ctx, cfg, infos, no_runs);
      break;
    case CONCRETE_LINEAR:
      concretize_linear(ctx, cfg, infos, no_runs);
      break;
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
      vmm_ensure_level(ctx->ptable, 3, infos[v].values[i]);
    }
  }

  /* actually set the values and ptes now
   *
   * TODO: should we do this now or at test run time?
   */
  for (uint64_t i = 0; i < no_runs; i++) {
    for (int v = 0; v < cfg->no_heap_vars; v++) {
      set_init_var(ctx, infos, v, i);
    }

    /* check that the concretization was successful before continuing */
    concretization_postcheck(ctx, cfg, infos, i);
  }
}