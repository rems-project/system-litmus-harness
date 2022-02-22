#include "lib.h"

u64 grp_num_tests(const litmus_test_group* grp) {
  u64 i = 0;
  if (grp->tests != NULL) {
    for (;;i++) {
      if (grp->tests[i] == NULL)
        break;
    }
  }
  return i;
}

u64 grp_num_groups(const litmus_test_group* grp) {
  u64 i = 0;
  if (grp->groups != NULL) {
    for (;;i++) {
      if (grp->groups[i] == NULL)
        break;
    }
  }
  return i;
}

u64 grp_num_total(const litmus_test_group* grp) {
  u64 tot = grp_num_tests(grp);
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

/* groups.c must define @all
*/
extern litmus_test_group grp_all;
void display_test_help(void) {
  printf("If none supplied, selects all enabled tests.\n");
  printf("Otherwise, runs all tests supplied in args that are one of:\n");
  display_help_for_grp(&grp_all);
}
