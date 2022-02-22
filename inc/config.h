#ifndef CONFIG_H
#define CONFIG_H

#include "argdef.h"

/* global configuration */
extern u64 NUMBER_OF_RUNS;
extern u64 RUNS_IN_BATCH;
extern u8 ENABLE_PGTABLE;
extern u8 ENABLE_PERF_COUNTS;
extern u8 RUN_FOREVER;

/** enable/disable collecting results histogram
 * and (if -t) print results direct to serial */
extern u8 ENABLE_RESULTS_HIST;

/** enable/disable printing of breakdown of register state
 * at end of test */
extern u8 ENABLE_RESULTS_OUTREG_PRINT;

/** enable/disable printing of warning of missing expected
 * outcomes */
extern u8 ENABLE_RESULTS_MISSING_SC_WARNING;

extern u8 VERBOSE;
extern u8 TRACE;
extern u8 DEBUG;

extern char*    collected_tests[100];
extern u64 collected_tests_count;
extern u8  ONLY_SHOW_MATCHES;

/* litmus-specific options */
typedef enum {
    SYNC_NONE,
    SYNC_ALL,
    SYNC_VA,
    SYNC_ASID,
} sync_type_t;

extern sync_type_t LITMUS_SYNC_TYPE;

typedef enum {
    AFF_NONE,
    AFF_RAND,
} aff_type_t;

extern aff_type_t LITMUS_AFF_TYPE;

typedef enum {
    SHUF_NONE,
    SHUF_RAND,
} shuffle_type_t;

extern shuffle_type_t LITMUS_SHUFFLE_TYPE;

typedef enum {
    CONCRETE_LINEAR,
    CONCRETE_RANDOM,
    CONCRETE_FIXED,
} concretize_type_t;

extern concretize_type_t LITMUS_CONCRETIZATION_TYPE;
extern char LITMUS_CONCRETIZATION_CFG[1024];

typedef enum {
    RUNNER_ARRAY,
    RUNNER_SEMI_ARRAY,
    RUNNER_EPHEMERAL,
} litmus_runner_type_t;

extern litmus_runner_type_t LITMUS_RUNNER_TYPE;

char* sync_type_to_str(sync_type_t ty);
char* aff_type_to_str(aff_type_t ty);
char* shuff_type_to_str(shuffle_type_t ty);
char* concretize_type_to_str(concretize_type_t ty);
char* runner_type_to_str(litmus_runner_type_t ty);

/* helper functions for displaying help */
void display_help_and_quit(argdef_t* args);
void display_help_for_and_quit(argdef_t* args, char* opt);
void display_help_show_tests(void);
void display_test_help(void);

/* read the input args and load the config vars */
void read_args(int argc, char** argv);
void init_cfg_state(void);

#endif /* CONFIG_H */