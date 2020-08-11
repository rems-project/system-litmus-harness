#include "lib.h"

extern void concretize_linear(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);
extern void concretize_standard(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);


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
    uint64_t* otherpte = ctx_pte(ctx, otherva);
    *pte = *otherpte;
  }
}

void concretize(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  switch (LITMUS_CONCRETIZATION_TYPE) {
    case CONCRETE_STANDARD:
      return concretize_standard(ctx, cfg, infos, no_runs);
    case CONCRETE_LINEAR:
      return concretize_linear(ctx, cfg, infos, no_runs);
    default:
      fail("! concretize: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
  }
}