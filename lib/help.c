#include "lib.h"

const char* HELPTEXT = 
  "Usage: litmus.exe [OPTION]... [TEST]... \n"
  "Run EL1/EL0 litmus tests\n"
  "\n"
  "Options: \n"
  "-h --help:       display this help text and quit.\n"
  "-d --debug:      enable debug output.\n"
  "-t --trace:      enable trace output.\n"
  "-p --pgtable:    switch to pagetable tests.\n"
  "--no-hist:       disable histogram output and print results as they are collected\n"
  "-nX:             set number of runs to X (default: 10000)\n";

void display_help_and_quit(void) {
  printf("%s", HELPTEXT);
  printf("\n");
  printf("Tests: \n");
  display_test_help();
  abort();
}