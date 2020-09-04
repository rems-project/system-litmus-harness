/**
 * concretization is the process of selecting virtual addresses for each variable
 * for each run of the harness.
 *
 * there are 2 stages of concretization:
 *  - selection of VAs
 *  - initialization of ptes/values
 *
 * there are roughly 3 ways to run a litmus test:
 *  - array:
 *    both phases of concretization happen at the beginning of time:
 *        SELECT VAs ITER #1
 *        INIT ITER #1
 *        SELECT VAs ITER #2
 *        INIT ITER #2
 *        ... then later ...
 *        RUN ITER #1
 *        RUN ITER #2
 *
 *  - ephemeral:
 *    both phases happen when the test is running:
 *      SELECT VAs ITER #1
 *      INIT ITER #1
 *      RUN ITER #1
 *      SELECT VAs ITER #2
 *      INIT ITER #2
 *      RUN ITER #2
 *
 *  - semi-array:
 *    selection of VAs and initialization happens upfront, like array.
 *    But PTEs need to be cleaned after each run since different runs may re-use the same page.
 */

#include "lib.h"

/* random */
extern void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, int run);
extern void concretize_random_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
extern void concretize_fixed_one(test_ctx_t* ctx, const litmus_test_t* cfg, int run);
extern void concretize_fixed_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);

/* linear */
extern void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);

/** given a var and an index perform the necessary initialization
 * to their pagetable entries
 *
 * assumes all the vars have their VAs assigned and the default
 * initial pagetable entries for all tests have been created
 */
void set_init_pte(test_ctx_t* ctx, uint64_t varidx, uint64_t idx) {
  if (! ENABLE_PGTABLE)
    return;

  var_info_t* vinfo = &ctx->heap_vars[varidx];
  uint64_t* va = vinfo->values[idx];
  uint64_t* pte = vmm_pte(ctx->ptable, (uint64_t)va);

  /* now if it was unmapped we can reset the last-level entry
  * to be invalid
  */
  if (vinfo->init_unmapped != 0) {
    *pte = 0;
  } else if (vinfo->init_ap != 0) {
    /* or if there were permissions, update those */
    desc_t desc = read_desc(*pte, 3);
    desc.attrs.AP = vinfo->init_ap;
    *pte = write_desc(desc);
  }

  /* set alias by copying the last-level pte OA
  * this means we must ensure that all VAs for this idx are chosen
  * before attempting to set_init_var
  */
  if (vinfo->alias) {
    uint64_t otheridx = idx_from_varname(ctx, vinfo->alias);
    uint64_t otherva = (uint64_t )ctx->heap_vars[otheridx].values[idx];

    /* do not copy attrs of otherpte */
    desc_t desc = read_desc(*pte, 3);
    desc.oa = PAGE(otherva) << PAGE_SHIFT;
    *pte = write_desc(desc);
  }

  /* flush TLB now before we perform the write below */
  if (LITMUS_SYNC_TYPE == SYNC_ALL) {
    vmm_flush_tlb();
  } else if (LITMUS_SYNC_TYPE == SYNC_ASID) {
    fail("--tlbsync=asid not supported\n");
  } else if (LITMUS_SYNC_TYPE == SYNC_VA) {
    vmm_flush_tlb_vaddr((uint64_t)va);
  }
}

/** given a var and an index perform the necessary initialization
 *
 * assumes all vars have their VAs assigned and the default initial pagetable
 * for the test has been created.
 *
 * this will, for each run, for each VA, set the PTE entry for that VA
 * and then write the initial value (if applicable).
 */
void set_init_var(test_ctx_t* ctx, uint64_t varidx, uint64_t idx) {
  var_info_t* vinfo = &ctx->heap_vars[varidx];

  uint64_t* va = vinfo->values[idx];
  set_init_pte(ctx, varidx, idx);

  if (ENABLE_PGTABLE) {
    attrs_t attrs = vmm_read_attrs(ctx->ptable, (uint64_t)va);
    if (attrs.AP == PROT_AP_RW_RWX) {
      *va = vinfo->init_value;
    }
  } else {
    *va = vinfo->init_value;
  }
}

void init_vars(test_ctx_t* ctx, const litmus_test_t* cfg, int run) {
  for (int v = 0; v < cfg->no_heap_vars; v++) {
    set_init_var(ctx, v, run);
  }

  /* check that the concretization was successful before continuing */
  concretization_postcheck(ctx, cfg, ctx->heap_vars, run);
}

void pick_concrete_addrs(test_ctx_t* ctx, const litmus_test_t* cfg, int run);

void concretize_one(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run) {
  switch (type) {
    case CONCRETE_LINEAR:
      fail("! concretize: cannot concretize_one with concretization=linear\n");
      break;
    case CONCRETE_RANDOM:
      concretize_random_one(ctx, cfg, run);
      break;
    case CONCRETE_FIXED:
      concretize_fixed_one(ctx, cfg, run);
      break;
    default:
      fail("! concretize_one: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }

  init_vars(ctx, cfg, run);
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
      concretize_random_all(ctx, cfg, no_runs);
      break;
    case CONCRETE_FIXED:
      concretize_fixed_all(ctx, cfg, no_runs);
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
      vmm_ensure_level(ctx->ptable, 3, (uint64_t)infos[v].values[i]);
    }
  }

  if (LITMUS_RUNNER_TYPE == RUNNER_ARRAY) {
    for (uint64_t i = 0; i < no_runs; i++) {
      init_vars(ctx, cfg, i);
    }
  }
}

void* concretize_allocate_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  switch (type) {
    case CONCRETE_LINEAR:
      break;
    case CONCRETE_RANDOM:
      break;
    case CONCRETE_FIXED:
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
    case CONCRETE_FIXED:
      break;
    default:
      fail("! concretize_free_st: got unexpected concretization type: %s (%s)\n", LITMUS_CONCRETIZATION_TYPE, (LITMUS_CONCRETIZATION_TYPE));
      break;
  }
}