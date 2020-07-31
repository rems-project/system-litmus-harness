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

/* litmus-specific options */
typedef enum {
    SYNC_NONE,
    SYNC_ALL,
    SYNC_ASID,
} sync_type_t;

extern sync_type_t LITMUS_SYNC_TYPE;

typedef enum {
    AFF_NONE,
    AFF_RAND,
} aff_type_t;

extern aff_type_t LITMUS_AFF_TYPE;

char* sync_type_to_str(sync_type_t ty);
char* aff_type_to_str(aff_type_t ty);

void display_help_show_tests(void);
void display_test_help(void);
void read_args(int argc, char** argv);

#endif /* CONFIG_H */