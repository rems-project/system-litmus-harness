#include "lib.h"

/* check all regions are in scope */
static void check_valid_regions(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos) {

}

/* if you specify PTE attributes on an unpinned region then
 * those PTE attributes might clash with other vars in the same page
 * hence */
static void check_regions_no_pte_attrs(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos) {
}

/* if two vars are allocated the same region
 * but supply different PTE mapping directives
 * then warn/fail */
static void check_consistent_pte_mappings(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos) {
  //for (int v1 = 0; v1 < cfg->no_heap_vars; v1++) {
  //  for (int v2 = v1+1; v2 < cfg->no_heap_vars; v2++) {
  //    uint8_t aliases =
  //      (infos[v2].alias == infos[v1].name) || (infos[v1].alias == infos[v1].name);
//
  //    uint8_t both_define_region =
  //      infos[v1].init_region_defined && infos[v2].init_region_defined;
//
  //    uint8_t both_same_region =
  //      infos[v1].init_region == infos[v2].init_region;
//
  //    if (
  //      !(aliases || (both_define_region && both_same_region))
  //    ) {
  //      continue;
  //    }
//
  //    if (
  //      (infos[v1].init_unmapped != infos[v2].init_unmapped)
  //      || (infos[v1].init_ap != infos[v2].init_ap)
  //    ) {
  //      fail("! concretization: for test %s variables '%s' and '%s' do not agree on PTE entries.\n",
  //        cfg->name,
  //        infos[v1].name,
  //        infos[v2].name);
  //    }
  //  }
  //}
}

/** check for nonsensical (bad) configurations */
void concretization_precheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos) {
  check_regions_no_pte_attrs(ctx, cfg, infos);
  check_consistent_pte_mappings(ctx, cfg, infos);
}


/* post checker
 *
 * this runs _after_ concretization to make sure it's valid
 */

/** check that the variables without defined regions were given non-overlapping region assignments this run
 *
 *  this ensures compatibility with previously written tests so that different variables are assigned different pages.
 */
void postcheck_compat_undefined_region_nooverlap(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
 // uint64_t COUNT_REGS [8] = { 0 };
 // for (int v = 0; v < cfg->no_heap_vars; v++) {
 //   if (! infos[v].init_region_defined) {
 //     COUNT_REGS[infos[v].curr_region] ++;
//
 //     if (COUNT_REGS[infos[v].curr_region] > 1) {
 //       fail("! concretization failed: multiple variables without defined regions failed to map to different regions\n");
 //     }
 //   }
 // }
}

void concretization_postcheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  postcheck_compat_undefined_region_nooverlap(ctx, cfg, infos, run);
}