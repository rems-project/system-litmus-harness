#include "lib.h"

extern void SB_pos(void);
extern void SB_dmbs(void);
extern void MP_pos(void);
extern void MP_dmbs(void);
extern void MP_dmb_svc(void);
extern void MP_dmb_eret(void);
extern void CoWT(void);
extern void CoWT_dsb(void);
extern void CoWT_dsbisb(void);
extern void CoWT_dsbsvctlbidsb(void);
extern void CoWTinv(void);
extern void CoWTinv_dsb(void);
extern void MPtimes_pos(void);
extern void WRCat_ctrl_dsb(void);

static const litmus_test_t TESTS[] = {
  {"SB+pos", SB_pos},
  {"SB+dmbs", SB_dmbs},
  {"MP+pos", MP_pos},
  {"MP+dmbs", MP_dmbs},
  {"MP+dmb+svc", MP_dmb_svc},
  {"MP+dmb+eret", MP_dmb_eret},
  {"MP.times+pos", MPtimes_pos, .requires_perf=1},
  {"CoWT", CoWT, .requires_pgtable=1},
  {"CoWT+dsb", CoWT_dsb, .requires_pgtable=1},
  {"CoWT+dsb-isb", CoWT_dsbisb, .requires_pgtable=1},
  {"CoWT+dsb-svc-tlbi-dsb", CoWT_dsbsvctlbidsb, .requires_pgtable=1},
  {"CoWT.inv", CoWTinv, .requires_pgtable=1},
  {"CoWT.inv+dsb", CoWTinv_dsb, .requires_pgtable=1},
  {"WRC.AT+ctrl+dsb", WRCat_ctrl_dsb, .requires_pgtable=1},
};

void display_test_help(void) {
  printf("If none supplied, selects all enabled tests.\n");
  printf("Otherwise, runs all tests supplied in args that are one of:\n");
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

static void run_test_fn(const litmus_test_t tst, uint8_t report_skip) {
  if (tst.requires_perf && (! ENABLE_PERF_COUNTS)) {
    if (report_skip)
      printf("! skipping \"%s\":  requires --perf\n", tst.fn_name);
    return;
  }

  if (tst.requires_pgtable && (! ENABLE_PGTABLE)) {
    if (report_skip)
      printf("! skipping \"%s\": requires --pgtable\n", tst.fn_name);
    return;
  }

  tst.fn();
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
