#ifndef FRONTEND_H
#define FRONTEND_H

#include "lib.h"

/** whether to actually run the tests or just display output
 */
extern uint8_t dry_run;

/** given some input, try print close matches for the user to see
 */
void print_closest(const litmus_test_group* grp, re_t* arg);

/** given a regex and a group
 * try run all the tests that match that regex inside that group
 */
void match_and_run(const litmus_test_group* grp, re_t* arg);


/** try print all tests that match the given regex
 */
void show_matches_only(const litmus_test_group* grp, re_t* arg);


uint64_t grp_num_tests(const litmus_test_group* grp);
uint64_t grp_num_groups(const litmus_test_group* grp);
uint64_t grp_num_total(const litmus_test_group* grp);

#endif /* FRONTEND_H */