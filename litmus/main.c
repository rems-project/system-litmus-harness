#include "lib.h"

extern litmus_test_t SB_pos;
extern litmus_test_t SB_dmbs;
extern litmus_test_t MP_dmbs;
extern litmus_test_t MP_pos;
extern litmus_test_t WRCat_ctrl_dsb;
extern litmus_test_t CoWT1_dsbtlbidsb;
extern litmus_test_t CoWT_dsbsvctlbidsb;
extern litmus_test_t MPRT_svcdsbtlbiisdsb_dsbisb;
extern litmus_test_t MPRT1_dsbtlbiisdsb_dsbisb;
extern litmus_test_t CoWTinv_dsb;
extern litmus_test_t CoWT_dsb;
extern litmus_test_t CoWT;
extern litmus_test_t CoWTinv;
extern litmus_test_t CoWT_dsbisb;
extern litmus_test_t MPRT_svcdsbtlbidsb_dsbisb;
extern litmus_test_t MPRT1_dsbtlbidsb_dsbisb;
extern litmus_test_t MP_dmb_eret;
extern litmus_test_t MP_dmb_svc;
extern litmus_test_t WRCtrt_dmbs;
extern litmus_test_t WRC1trt_dsbtlbiisdsb_dmb;
extern litmus_test_t CoTR;


static const litmus_test_t* TESTS[] = {
  &SB_pos,
  &SB_dmbs,
  &MP_dmbs,
  &MP_pos,
  &WRCat_ctrl_dsb,
  &CoWT1_dsbtlbidsb,
  &CoWT_dsbsvctlbidsb,
  &MPRT_svcdsbtlbiisdsb_dsbisb,
  &MPRT1_dsbtlbiisdsb_dsbisb,
  &CoWTinv_dsb,
  &CoWT_dsb,
  &CoWT,
  &CoWTinv,
  &CoWT_dsbisb,
  &CoTR,
  &MPRT_svcdsbtlbidsb_dsbisb,
  &MPRT1_dsbtlbidsb_dsbisb,
  &MP_dmb_eret,
  &MP_dmb_svc,
  &WRCtrt_dmbs,
  &WRC1trt_dsbtlbiisdsb_dmb,
};

void display_test_help(void) {
  printf("If none supplied, selects all enabled tests.\n");
  printf("Otherwise, runs all tests supplied in args that are one of:\n");
  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t*); i++) {
    litmus_test_t const* t = TESTS[i];
    printf(" %s", t->name);

    if (t->requires_perf) {
      printf(" (requires --perf)");
    }

    if (t->requires_pgtable) {
      printf(" (requires --pgtable)");
    }

    printf("\n");
  }
}

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

  run_test(tst);
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
static void __find_closest(char* arg) {
  char const* smallest = NULL;
  int small_diff = 0;
  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t*); i++) {
    char* name = (char*)TESTS[i]->name;
    int diff = strdiff(arg, name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = name;
        small_diff = diff;
      }
    }
  }

  if (smallest != NULL) {
    printf("Did you mean \"%s\" ?\n", smallest);
  }
}

static void match_and_run(char* arg) {
  uint8_t found = 0;

  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t*); i++) {
    if (strcmp(arg, "*") || strcmp(arg, TESTS[i]->name)) {
      run_test_fn(TESTS[i], !strcmp(arg, "*"));
      found = 1;
    }
  }

  if (! found) {
    printf("! err: unknown test: \"%s\"\n", arg);
    __find_closest(arg);
    abort();
  }
}

int main(int argc, char** argv) {
  if (collected_tests_count == 0) {
    match_and_run("*");
  } else {
    for (int i = 0; i < collected_tests_count; i++) {
      match_and_run(collected_tests[i]);
    }
  }
  return 0;
}
