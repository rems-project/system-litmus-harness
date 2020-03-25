#include "lib.h"

extern void MP_pos(void);
extern void MP_dmbs(void);
extern void MP_dmb_svc(void);
extern void MP_dmb_eret(void);
extern void CoWT(void);
extern void CoWT_dsb(void);
extern void CoWTinv(void);
extern void CoWTinv_dsb(void);
extern void MPtimes_pos(void);

static const litmus_test_t TESTS[] = {
  {"MP+pos", MP_pos},
  {"MP+dmbs", MP_dmbs},
  {"MP+dmb+svc", MP_dmb_svc},
  {"MP+dmb+eret", MP_dmb_eret},
  {"MP.times+pos", MPtimes_pos, .requires_perf=1},
  {"CoWT", CoWT, .requires_pgtable=1},
  {"CoWT+dsb", CoWT_dsb, .requires_pgtable=1},
  {"CoWT.inv", CoWTinv, .requires_pgtable=1},
  {"CoWT.inv+dsb", CoWTinv_dsb, .requires_pgtable=1},
};

void display_test_help(void) {
  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t); i++) {
    litmus_test_t const* t = &TESTS[i];
    printf(" %s", t->fn_name);

    if (t->requires_perf) {
      printf(" (requires --perf)");
    }

    if (t->requires_pgtable) {
      printf(" (requires --pgtable)");
    }

    printf("\n");
  }
}

static void run_test_fn(const litmus_test_t tst) {
  if (tst.requires_perf && (! ENABLE_PERF_COUNTS)) {
    printf("! skipping \"%s\":  requires --perf\n", tst.fn_name);
    return;
  }

  if (tst.requires_pgtable && (! ENABLE_PGTABLE)) {
    printf("! skipping \"%s\": requires --pgtable\n", tst.fn_name);
    return;
  }

  tst.fn();
}

/** estimate a numeric difference between two strings
 * horribly inefficient but who cares
 */
static int strdiff(char* w1, char* w2) {
  char c = *w1;
  char d = *w2;
  if (c == d && c == '\0') {
    return 0;
  } else if (c == d) {
    return strdiff(w1+1, w2+1);
  } else if (c == '\0' || d =='\0') {
    return MAX(strlen(w1), strlen(w2)) - MIN(strlen(w1), strlen(w2));
  } else {
    int cd = MAX(c,d) - MIN(c,d);
    int diff0 = strdiff(w1+1, w2+1)+1;
    int diff1 = strdiff(w1+1, w2)+2;
    int diff2 = strdiff(w1, w2+1)+2;
    return MIN(diff0, MIN(diff1, diff2)) + cd;
  }
}

/** if a test cannnot be found
 * find a nearby one and say that instead
 */
static void __find_closest(char* arg) {
  char const* smallest = NULL;
  int small_diff;
  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t); i++) {
    int diff = strdiff(arg, (char*)TESTS[i].fn_name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = TESTS[i].fn_name;
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

  for (int i = 0; i < sizeof(TESTS)/sizeof(litmus_test_t); i++) {
    if (strcmp(arg, "*") || strcmp(arg, TESTS[i].fn_name)) {
      run_test_fn(TESTS[i]);
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
