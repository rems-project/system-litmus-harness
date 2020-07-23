#include "lib.h"

const char* HELPTEXT =
  "Usage: litmus.exe [OPTION]... [TEST]... \n"
  "Run EL1/EL0 litmus tests\n"
  "\n"
  "Options: \n"
  "-h --help:                 display this help text and quit.\n"
  "-nX:                       set number of runs to X (default: 10000)\n"
  "-d --debug:                enable debug output.\n"
  "-t --trace:                enable trace output.\n"
  "--verbose / --no-verbose:  enabled/disable verbose output.\n"
  "--pgtable / --no-pgtable:  enabled/disable pagetable tests.\n"
  "--no-hist:                 disable histogram output and print results as they are collected\n"
  "--show:                    display the list of tests and quit\n"
  "-sX                        initial seed (default: none)\n"
  ;

void display_help_and_quit(void) {
  printf("%s", HELPTEXT);
  abort();
}

void display_help_show_tests(void) {
  printf("Tests: \n");
  display_test_help();
  printf("\n");
  abort();
}