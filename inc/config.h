#ifndef CONFIG_H
#define CONFIG_H

/* global configuration */
extern uint64_t NUMBER_OF_RUNS;
extern uint8_t ENABLE_PGTABLE;
extern uint8_t ENABLE_PERF_COUNTS;

/** disable collecting results histogram
 * and (if -t) print results direct to serial */
extern uint8_t DISABLE_RESULTS_HIST;

extern uint8_t VERBOSE;
extern uint8_t TRACE;
extern uint8_t DEBUG;

extern char*  collected_tests[100];
extern int    collected_tests_count;
extern uint8_t ONLY_SHOW_MATCHES;


void display_help_and_quit(void);
void display_test_help(void);
void read_args(int argc, char** argv);

#endif /* CONFIG_H */