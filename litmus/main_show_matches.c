#include "lib.h"
#include "frontend.h"

static void _print_all_tests(const litmus_test_group* grp) {
    for (int i = 0; i < grp_num_tests(grp); i++) {
        printf(" %s\n", grp->tests[i]->name);
    }

    for (int i = 0; i < grp_num_groups(grp); i++) {
        _print_all_tests(grp->groups[i]);
    }
}

static void _print_matches_group(const litmus_test_group* grp, re_t* arg) {
  if (re_matches(arg, grp->name)) {
    printf("(%s)\n", grp->name);
    _print_all_tests(grp);
  }

  for (u64 i = 0; i < grp_num_groups(grp); i++) {
    _print_matches_group(grp->groups[i], arg);
  }
}

static void _print_matches_test(const litmus_test_group* grp, re_t* arg) {
  for (u64 i = 0; i < grp_num_tests(grp); i++) {
    if (re_matches(arg, grp->tests[i]->name)) {
      printf(" %s\n", grp->tests[i]->name);
    }
  }

  for (u64 i = 0; i < grp_num_groups(grp); i++) {
    _print_matches_test(grp->groups[i], arg);
  }
}

void show_matches_only(const litmus_test_group* grp, re_t* arg) {
    if (*arg->original_expr == '@') {
        _print_matches_group(grp, arg);
    } else {
        _print_matches_test(grp, arg);
    }
}