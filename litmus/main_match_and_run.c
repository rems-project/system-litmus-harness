#include "lib.h"
#include "frontend.h"

static void run_test_fn(const litmus_test_t* tst, u8 report_skip) {
  if (tst->requires_perf && (!ENABLE_PERF_COUNTS)) {
    if (report_skip)
      warning(WARN_SKIP_TEST, "skipping \"%s\":  requires --perf\n", tst->name);
    return;
  }

  if (tst->requires_pgtable && (!ENABLE_PGTABLE)) {
    if (report_skip)
      warning(WARN_SKIP_TEST, "skipping \"%s\": requires --pgtable\n", tst->name);
    return;
  }

  if (tst->requires_debug && (!DEBUG)) {
    if (report_skip)
      warning(WARN_SKIP_TEST, "skipping \"%s\": requires -d\n", tst->name);
    return;
  }

  if (!dry_run) {
    run_test(tst);
  }
}

static void run_all_group(const litmus_test_group* grp) {
  for (u64 i = 0; i < grp_num_tests(grp); i++) {
    run_test_fn(grp->tests[i], dry_run);
  }

  for (u64 i = 0; i < grp_num_groups(grp); i++) {
    run_all_group(grp->groups[i]);
  }
}

static u8 __match_and_run_group(const litmus_test_group* grp, re_t* arg) {
  u8 found_match = 0;

  if (re_matches(arg, grp->name)) {
    run_all_group(grp);
    found_match = 1;
  }

  for (u64 i = 0; i < grp_num_groups(grp); i++) {
    if (__match_and_run_group(grp->groups[i], arg)) {
      found_match = 1;
    }
  }

  return found_match;
}

static u8 __match_and_run_test(const litmus_test_group* grp, re_t* arg) {
  u8 found_match = 0;
  for (u64 i = 0; i < grp_num_tests(grp); i++) {
    if (re_matches(arg, grp->tests[i]->name)) {
      run_test_fn(grp->tests[i], 0);
      found_match = 1;
    }
  }

  for (u64 i = 0; i < grp_num_groups(grp); i++) {
    if (__match_and_run_test(grp->groups[i], arg)) {
      found_match = 1;
    }
  }

  return found_match;
}

void match_and_run(const litmus_test_group* grp, re_t* arg) {
  u8 found = 0;

  if (*arg->original_expr == '@') {
    found = __match_and_run_group(grp, arg);
  } else {
    found = __match_and_run_test(grp, arg);
  }

  if (!found) {
    warning(WARN_ALWAYS, "no test matches: \"%s\"\n", arg->original_expr);
    print_closest(grp, arg);
    abort();
  }
}
