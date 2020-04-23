#include "lib.h"

extern litmus_test_t 
    SB_pos,
    SB_dmbs,
    MP_dmbs,
    MP_pos,
    WRC_pos,
    WRC_addrs,
    WRCat_ctrl_dsb,
    CoWT1_dsbtlbidsb,
    CoWT_dsbsvctlbidsb,
    MPRT_svcdsbtlbiisdsb_dsbisb,
    MPRT1_dsbtlbiisdsb_dsbisb,
    CoWTinv_dsb,
    CoWT_dsb,
    CoWT,
    CoWTinv,
    CoWT_dsbisb,
    MPRT_svcdsbtlbidsb_dsbisb,
    MPRT1_dsbtlbidsb_dsbisb,
    MPRT1_dsbtlbialldsb_dsbisb,
    MPRT1_dsbtlbiisdsbtlbiisdsb_dsbisb,
    MP_dmb_eret,
    MP_dmb_svc,
    MPRTinv_dmb_addr,
    WRCtrr_addr_dmb,
    WRCtrrinv_addrs,
    WRCtrt_addr_dmb,
    WRCtrt_dmbs,
    WRCtrt_dsbisbs,
    WRCtrtinv_dsbisbs,
    WRCtrtinv_po_dmb,
    WRCtrtinv_po_addr,
    WRC1trt_dsbtlbiisdsb_dmb,
    CoTR,
    CoTRinv,
    CoTRinv_dsbisb,
    CoTR_addr,
    CoTR_dmb,
    CoTR_dsb,
    CoTR_dsbisb,
    CoTR1tlbi_dsbdsbisb,
    CoWinvT,
    CoWinvT1_dsbtlbidsb,
    ISA2trr_dmb_po_dmb;


const litmus_test_group data_group = {
  .name="@data",
  .tests = (const litmus_test_t*[]){
    &MP_pos,
    &MP_dmbs,
    &SB_pos,
    &SB_dmbs,
    &WRC_pos,
    &WRC_addrs,
    NULL,
  }
};

const litmus_test_group exc_group = {
  .name = "@exc",
  .tests = (const litmus_test_t*[]){
    &MP_dmb_svc,
    &MP_dmb_eret,
    NULL,
  }
};

const litmus_test_group pgtable_group = {
  .name = "@pgtable",
  .tests = (const litmus_test_t*[]){
    &WRCat_ctrl_dsb,
    &CoWT1_dsbtlbidsb,
    &CoWT_dsbsvctlbidsb,
    &MPRT_svcdsbtlbiisdsb_dsbisb,
    &MPRT1_dsbtlbiisdsb_dsbisb,
    &MPRT1_dsbtlbialldsb_dsbisb,
    &MPRT1_dsbtlbiisdsbtlbiisdsb_dsbisb,
    &CoWTinv_dsb,
    &CoWT_dsb,
    &CoWT,
    &CoWTinv,
    &CoWinvT,
    &CoWinvT1_dsbtlbidsb,
    &CoWT_dsbisb,
    &CoTR,
    &CoTRinv,
    &CoTRinv_dsbisb,
    &CoTR_addr,
    &CoTR_dmb,
    &CoTR_dsb,
    &CoTR_dsbisb,
    &CoTR1tlbi_dsbdsbisb,
    &MPRT_svcdsbtlbidsb_dsbisb,
    &MPRT1_dsbtlbidsb_dsbisb,
    &MP_dmb_eret,
    &MP_dmb_svc,
    &MPRTinv_dmb_addr,
    &WRCtrr_addr_dmb,
    &WRCtrrinv_addrs,
    &WRCtrt_addr_dmb,
    &WRCtrt_dmbs,
    &WRCtrt_dsbisbs,
    &WRCtrtinv_dsbisbs,
    &WRC1trt_dsbtlbiisdsb_dmb,
    &WRCtrtinv_po_dmb,
    &WRCtrtinv_po_addr,
    &ISA2trr_dmb_po_dmb,
    NULL,
  }
};

const litmus_test_group all = {
  .name="@all",
  .groups = (const litmus_test_group*[]){
    &data_group,
    &exc_group,
    &pgtable_group,
    NULL,
  }
};

litmus_test_t check1, check2, check3, MPtimes_pos;
const litmus_test_group timing_group = {
  .name="@timing",
  .tests = (const litmus_test_t*[]){
    &MPtimes_pos,
    NULL,
  }
};

const litmus_test_group checks = {
  .name="@checks",
  .tests = (const litmus_test_t*[]){
    &check1,
    &check2,
    &check3,
    NULL,
  }
};

const litmus_test_group _real_all = {
  .name=NULL,
  .groups = (const litmus_test_group*[]){
    &all,
    &checks,
    &timing_group,
    NULL,
  },
};

uint64_t grp_num_tests(const litmus_test_group* grp) {
  uint64_t i = 0;
  if (grp->tests != NULL) {
    for (;;i++) {
      if (grp->tests[i] == NULL)
        break;
    }
  }
  return i;
}

uint64_t grp_num_groups(const litmus_test_group* grp) {
  uint64_t i = 0;
  if (grp->groups != NULL) {
    for (;;i++) {
      if (grp->groups[i] == NULL)
        break;
    }
  }
  return i;
}

uint64_t grp_num_total(const litmus_test_group* grp) {
  uint64_t tot = grp_num_tests(grp);
  for (int i = 0; i < grp_num_groups(grp); i++) {
    tot += grp_num_total(grp->groups[i]);
  }
  return tot;
}

void display_help_for_grp(const litmus_test_group* grp) {
  if (grp->name)
    printf("%s:\n", grp->name);

  for (int i = 0; i < grp_num_tests(grp); i++) {
    const litmus_test_t* t = grp->tests[i];
    printf(" %s", t->name);

    if (t->requires_perf) {
      printf(" (requires --perf)");
    }

    if (t->requires_pgtable) {
      printf(" (requires --pgtable)");
    }

    if (t->requires_debug) {
      printf(" (requires -d)");
    }

    printf("\n");
  }

  for (int i = 0; i < grp_num_groups(grp); i++) {
    display_help_for_grp(grp->groups[i]);
  }
}

void display_test_help(void) {
  printf("If none supplied, selects all enabled tests.\n");
  printf("Otherwise, runs all tests supplied in args that are one of:\n");
  display_help_for_grp(&all);
}

/** if 1 then don't run just check */
static uint8_t dry_run = 0;

static void run_test_fn(const litmus_test_t* tst, uint8_t report_skip) {
  if (tst->requires_perf && (! ENABLE_PERF_COUNTS)) {
    if (report_skip)
      printf("! skipping \"%s\":  requires --perf\n", tst->name);
    return;
  }

  if (tst->requires_pgtable && (! ENABLE_PGTABLE)) {
    if (report_skip)
      printf("! skipping \"%s\": requires --pgtable\n", tst->name);
    return;
  }

  if (tst->requires_debug && (! DEBUG)) {
    if (report_skip)
      printf("! skipping \"%s\": requires -d\n", tst->name);
    return;
  }

  if (! dry_run) {
    run_test(tst);
  }
}


#define LOWER(c) (((c) < 90 && (c) > 65 ? (c)+32 : (c)))
/** estimate a numeric difference between two strings
 * horribly inefficient but who cares
 * 
 * This is optimized for litmus test names, which are often the same jumble of letters but in slightly different orders
 */
static int strdiff(char* w1, char* w2) {
  int sw1 = strlen(w1);
  int sw2 = strlen(w2);
  int diff = 0;
  for (int i = 0; i < sw1; i++) {
    char c, d, e;

    if (w2[i] == '\0') {
      return diff + 10*(sw1 - sw2);
    }

    c = d  = e = w2[i];
    if (0 < (i - 1) && (i - 1) < sw2) {
      c = w2[i - 1];
    }

    if ((i + 1) < sw2) {
      e = w2[i + 1];
    }

    #define CMPCHR(a,b) \
        (a==b ? 0 : \
          (LOWER(a)==LOWER(b) ? 1 : \
            ((a == '+' || b == '+') ? 2 : \
              (MAX(LOWER(a),LOWER(b))-MIN(LOWER(a),LOWER(b))))))

    d = w2[i];
    char x = w1[i];
    diff += MIN(CMPCHR(x,d), MIN(1+CMPCHR(x,c), 1+CMPCHR(x,e)));
  }

  return diff + sw2 - sw1;
}

/** if a test cannnot be found
 * find a nearby one and say that instead
 */
static const char* __find_closest_str(const litmus_test_group* grp, char* arg) {
  char const* smallest = NULL;
  int small_diff = 0;
  
  for (int i = 0; i < grp_num_tests(grp); i++) {
    char* name = (char*)grp->tests[i]->name;
    int diff = strdiff(arg, name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = name;
        small_diff = diff;
      }
    }
  }

  if (grp->name) {
    int diff = strdiff(arg, (char*)grp->name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = grp->name;
        small_diff = diff;
      }
    }
  }

  for (int i = 0; i < grp_num_groups(grp); i++) {
    const char* _sub_smallest = __find_closest_str(grp->groups[i], arg);
    int diff = strdiff(arg, (char*)_sub_smallest);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = _sub_smallest;
        small_diff = diff;
      }
    }
  }

  return smallest;
}

static void __find_closest(const litmus_test_group* grp, char* arg) {
  const char* smallest = __find_closest_str(grp, arg);

  if (smallest != NULL) {
    printf("Did you mean \"%s\" ?\n", smallest);
  }
}

static void run_all_group(const litmus_test_group* grp) {
  for (uint64_t i = 0; i < grp_num_tests(grp); i++) {
    run_test_fn(grp->tests[i], dry_run);
  }

  for (uint64_t i = 0; i < grp_num_groups(grp); i++) {
    run_all_group(grp->groups[i]);
  }
}

static uint8_t __match_and_run_group(const litmus_test_group* grp, char* arg) {
  if (strcmp(arg, grp->name)) {
    run_all_group(grp);
    return 1;
  }

  for (uint64_t i = 0; i < grp_num_groups(grp); i++) {
    if (__match_and_run_group(grp->groups[i], arg)) {
      return 1;
    }
  }

  return 0;
}

static uint8_t __match_and_run_test(const litmus_test_group* grp, char* arg) {
  for (uint64_t i = 0; i < grp_num_tests(grp); i++) {
    if (strcmp(arg, grp->tests[i]->name)) {
      run_test_fn(grp->tests[i], 0);
      return 1;
    }
  }

  for (uint64_t i = 0; i < grp_num_groups(grp); i++) {
    if (__match_and_run_test(grp->groups[i], arg)) {
      return 1;
    }
  }

  return 0;
}

static void match_and_run(const litmus_test_group* grp, char* arg) {
  uint8_t found = 0;

  if (*arg == '@') {
    found = __match_and_run_group(grp, arg);
  } else {
    found = __match_and_run_test(grp, arg);
  }

  if (! found) {
    printf("! err: unknown test: \"%s\"\n", arg);
    __find_closest(grp, arg);
    abort();
  }
}

int main(int argc, char** argv) {
  if (collected_tests_count == 0) {
    match_and_run(&all, "@all");  /* default to @all */
  } else {
    /* first do a dry run, without actually running the functions
      * just to validate the arguments */
    for (uint8_t r = 0; r <= 1; r++) {
      dry_run = 1 - r;

      for (int i = 0; i < collected_tests_count; i++) {
        /* @_real_all is a phony group and contains things that should not be run when using @all
        * but can be specifcally selected */
        match_and_run(&_real_all, collected_tests[i]);
      }
    }
  }
  return 0;
}
