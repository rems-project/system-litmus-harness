#include <stdint.h>

#include "lib.h"
#include "argdef.h"

/* global configuration options + default values */
uint64_t NUMBER_OF_RUNS = 10000UL;
uint8_t ENABLE_PGTABLE = 1;  /* start enabled */
uint8_t ENABLE_PERF_COUNTS = 0;

uint8_t ENABLE_RESULTS_HIST = 1;

uint8_t VERBOSE = 1;  /* start verbose */
uint8_t TRACE = 0;
uint8_t DEBUG = 0;

uint8_t ONLY_SHOW_MATCHES = 0;

char* collected_tests[100];
int   collected_tests_count;

sync_type_t LITMUS_SYNC_TYPE = SYNC_ALL;
aff_type_t LITMUS_AFF_TYPE = AFF_RAND;

char* sync_type_to_str(sync_type_t ty) {
  switch (ty) {
    case SYNC_NONE:
      return "none";
    case SYNC_ALL:
      return "all";
    case SYNC_ASID:
      return "asid";
    default:
      return "unknown";
  }
}

char* aff_type_to_str(aff_type_t ty) {
  switch (ty) {
    case AFF_NONE:
      return "none";
    case AFF_RAND:
      return "rand";
    default:
      return "unknown";
  }
}

static void help(char* opt) {
  if (opt == NULL || *opt == '\0')
    display_help_and_quit(&ARGS);
  else
    display_help_for_and_quit(&ARGS, opt);
}

static void n(char* x) {
  int Xn = atoi(x);
  NUMBER_OF_RUNS = Xn;
}

static void s(char* x) {
  int Xn = atoi(x);
  INITIAL_SEED = Xn;
}

static void t(char* x) {
  TRACE = 1;
}

static void d(char* x) {
  DEBUG = 1;
}

static void show(char* x) {
  display_help_show_tests();
}

static void q(char* x) {
  if (x != NULL && *x != '\0') {
    fail("-q/--quiet did not expect an argument.\n");
  }

  VERBOSE = 0;
  DEBUG = 0;
  TRACE = 0;
}
argdef_t ARGS = (argdef_t){
  .version=VERSION,
  .args=(const argdef_arg_t*[]){
    OPT(
      "-h",
      "--help",
      help,
      "display this help text and quit\n"
      "\n"
      "displays help text.\n"
      "--help=foo will display detailed help for foo.",
      .show_help_both=1
    ),
    OPT(
      NULL,
      "--show",
      show,
      "show list of tests and quit\n"
      "\n"
      "displays the complete list of the compiled tests and their groups, then quits.",
      .only_action=1
    ),
    OPT(
      "-n",
      NULL,
      n,
      "number of runs per test\n"
      "\n"
      "sets the number of runs per test\n"
      "X must be an integer (default: 10k).\n"
      "Examples: \n"
      " ./litmus.exe -n300\n"
      " ./litmus.exe -n10k\n"
      " ./litmus.exe -n1M\n"
      "\n"
      "Note that currently 1M is likely to fail due to over-allocation of results.\n"
    ),
    FLAG(
      "-p",
      "--pgtable",
      ENABLE_PGTABLE,
      "enable/disable pagetable tests\n"
    ),
    FLAG(
      NULL,
      "--perf",
      ENABLE_PERF_COUNTS,
      "enable/disable performance tests\n"
    ),
    FLAG(
      "-d",
      "--debug",
      DEBUG,
      "enable/disable debugging\n"
    ),
    FLAG(
      NULL,
      "--verbose",
      VERBOSE,
      "enable/disable verbose output\n"
    ),
    OPT(
      "-q",
      "--quiet",
      q,
      "quiet mode",
      .only_action=1
    ),
    OPT(
      "-t",
      "--trace",
      t,
      "tracing mode",
      .only_action=1
    ),
    OPT(
      "-d",
      "--debug",
      d,
      "debugging mode",
      .only_action=1
    ),
    FLAG(
      NULL,
      "--hist",
      ENABLE_RESULTS_HIST,
      "enable/disable results histogram collection\n"
    ),
    OPT(
      "-s",
      "--seed",
      s,
      "initial seed\n"
      "\n"
      "at the beginning of each test, a random seed is selected.\n"
      "this seed can be forced with --seed=12345 which ensures some level of determinism."
    ),
    ENUMERATE(
      "--sync",
      LITMUS_SYNC_TYPE,
      sync_type_t,
      3,
      ARR((const char*[]){"none", "asid", "all"}),
      ARR((sync_type_t[]){SYNC_NONE, SYNC_ASID, SYNC_ALL}),
      "type of tlb synchronization\n"
      "\n"
      "none:  no synchronization of the TLB (incompatible with --pgtable).\n"
      "all: always flush the entire TLB in-between tests.\n"
      "asid: (EXPERIMENTAL) assign each test run an ASID and only flush that \n"
    ),
    ENUMERATE(
      "--aff",
      LITMUS_AFF_TYPE,
      aff_type_t,
      2,
      ARR((const char*[]){"none", "rand"}),
      ARR((aff_type_t[]){AFF_NONE, AFF_RAND}),
      "type of affinity control\n"
      "\n"
      "none: Thread 0 is pinned to CPU0, Thread 1 to CPU1 etc.\n"
      "rand: Thread 0 is pinned to vCPU0, etc.  vCPUs may migrate between CPUs"
      " in-between tests.\n"
    ),
    NULL,  /* nul terminated list */
  }
};

void read_args(int argc, char** argv) {
  argparse_read_args(&ARGS, argc, argv);
}