#include <stdint.h>

#include "lib.h"


/* global configuration options + default values */
uint64_t NUMBER_OF_RUNS = 10000UL;
uint8_t ENABLE_PGTABLE = 0;
uint8_t ENABLE_PERF_COUNTS = 0;

uint8_t DISABLE_RESULTS_HIST = 0;

uint8_t VERBOSE = 1;  /* start verbose */
uint8_t TRACE = 0;
uint8_t DEBUG = 0;

uint8_t ONLY_SHOW_MATCHES = 0;

char* collected_tests[100];
int   collected_tests_count;

static void read_arg(char* w) {
  char* word = w;

  switch (w[0]) {
    case '\0':
      return;

    case '-':
      switch (w[1]) {
        case 't':
          TRACE = 1;
          return;

        case 'd':
          DEBUG = 1;
          return;

        case 'p':
          ENABLE_PGTABLE = 1;
          return;

        case 'n':
          word += 2;
          NUMBER_OF_RUNS = atoi(word);
          return;

        case 'm':
          ONLY_SHOW_MATCHES = 1;
          return;

        case 'h':
          display_help_and_quit();
          return;

        case 's':
          word += 2;
          INITIAL_SEED = atoi(word);
          return;

        case '-':
          word = w+2;
          if (strcmp(word, "help")) {
            display_help_and_quit();
            return;
          } else if (strcmp(word, "perf")) {
            ENABLE_PERF_COUNTS = 1;
            return;
          } else if (strcmp(word, "no-hist")) {
            DISABLE_RESULTS_HIST = 1;
            return;
          } else if (strcmp(word, "trace")) {
            TRACE = 1;
            return;
          } else if (strcmp(word, "debug")) {
            DEBUG = 1;
            return;
          } else if (strcmp(word, "pgtable")) {
            ENABLE_PGTABLE = 1;
            return;
          } else if (strcmp(word, "no-pgtable")) {
            ENABLE_PGTABLE = 0;
            return;
          } else if (strcmp(word, "verbose")) {
            VERBOSE = 1;
            return;
          } else if (strcmp(word, "no-verbose")) {
            VERBOSE = 0;
            return;
          } else if (strcmp(word, "quiet")) {
            VERBOSE = 0;
            TRACE = 0;
            DEBUG = 0;
            return;
          }
          break;

        default:
          goto read_arg_fail;
      }
      break;

    default:
      collected_tests[collected_tests_count++] = w;
      return;
  }

read_arg_fail:
  printf("! err: unknown argument \"%s\"\n", w);
  abort();
  return;
}

void read_args(int argc, char** argv) {
  for (int i = 0; i < argc; i++) {
    read_arg(argv[i]);
  }
}