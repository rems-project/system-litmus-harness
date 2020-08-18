#include "lib.h"

static void fail_postcheck(test_ctx_t* ctx, const litmus_test_t* cfg, int run, const char* fmt, ...) {
  char buffer[1024];
  char* out = &buffer[0];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(out, 1, fmt, ap);
  va_end(ap);
  fail("! error: concretization of test %s failed on run %d: %s\n", cfg->name, run, &buffer[0]);
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

#define IS_SAME_PAGE(va1, va2) (PAGE((uint64_t)(va1)) == PAGE((uint64_t)(va2)))

static void postcheck_overlapping_pages_pte_agree(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  for (int varidx1 = 0; varidx1 < cfg->no_heap_vars; varidx1++) {
    for (int varidx2 = varidx1+1; varidx2 < cfg->no_heap_vars; varidx2++) {
      var_info_t* v1 = &infos[varidx1];
      var_info_t* v2 = &infos[varidx2];

      if (IS_SAME_PAGE(v1->values[run], v2->values[run])) {
        if (v1->init_unmapped != v2->init_unmapped) {
          fail_postcheck(ctx, cfg, run, "%s and %s were mapped to same page but only one is mapped.\n", v1->name, v2->name);
        }

        if (v1->init_ap != v2->init_ap) {
          fail_postcheck(ctx, cfg, run, "%s and %s were mapped to same page but have different APs (%d and %d respectively).\n", v1->name, v2->name, v1->init_ap, v2->init_ap);
        }
      }
    }
  }
}

static void postcheck_unpinned_not_same_page(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  for (int varidx1 = 0; varidx1 < cfg->no_heap_vars; varidx1++) {
    for (int varidx2 = varidx1+1; varidx2 < cfg->no_heap_vars; varidx2++) {
      var_info_t* v1 = &infos[varidx1];
      var_info_t* v2 = &infos[varidx2];

      if ((! v1->init_region_pinned) && (! v2->init_region_pinned) && IS_SAME_PAGE(v1->values[run], v2->values[run])) {
        fail_postcheck(ctx, cfg, run, "%s and %s were mapped to same page but were both unpinned\n", v1->name, v2->name);
      }
    }
  }
}

static void postcheck_unpinned_different_page_unrelated_vars(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  for (int varidx1 = 0; varidx1 < cfg->no_heap_vars; varidx1++) {
    for (int varidx2 = varidx1+1; varidx2 < cfg->no_heap_vars; varidx2++) {
      var_info_t* v1 = &infos[varidx1];
      var_info_t* v2 = &infos[varidx2];

      if (IS_SAME_PAGE(v1->values[run], v2->values[run])) {
        if (
             ((! v1->init_region_pinned) && (  v2->init_region_pinned) && v2->pin_region_var != v1->name)
          || ((  v1->init_region_pinned) && (! v2->init_region_pinned) && v1->pin_region_var != v2->name)
        ) {
          fail_postcheck(ctx, cfg, run, "%s and %s were mapped to same page but are unrelated\n", v1->name, v2->name);
        }
      }
    }
  }
}

void concretization_postcheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int run) {
  postcheck_overlapping_pages_pte_agree(ctx, cfg, infos, run);
  postcheck_unpinned_not_same_page(ctx, cfg, infos, run);
  postcheck_unpinned_different_page_unrelated_vars(ctx, cfg, infos, run);
}