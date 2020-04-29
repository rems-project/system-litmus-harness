#include "lib.h"
#include "frontend.h"

/* defined in the auto-generated groups.c */
extern litmus_test_group grp_all;

/** if 1 then don't run just check */
uint8_t dry_run = 0;

int main(int argc, char** argv) {
  if (ONLY_SHOW_MATCHES) {
    for (int i = 0; i < collected_tests_count; i++) {
      re_t* re = re_compile(collected_tests[i]);
      show_matches_only(&grp_all, re);
      re_free(re);
    }

    return 0;
  }

  if (collected_tests_count == 0) {
    re_t* re = re_compile("@all");
    match_and_run(&grp_all, re);  /* default to @all */
    re_free(re);
  } else {
    /* first do a dry run, without actually running the functions
      * just to validate the arguments */
    for (uint8_t r = 0; r <= 1; r++) {
      dry_run = 1 - r;

      for (int i = 0; i < collected_tests_count; i++) {
        re_t* re = re_compile(collected_tests[i]);
        match_and_run(&grp_all, re);
        re_free(re);
      }
    }
  }
  return 0;
}
