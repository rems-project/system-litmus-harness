#include "lib.h"

const char* HELPTEXT = 
  "Usage: litmus.exe [-h] [-p] [-t] [-d] [-nX]\n"
  "-h:      display this help text and quit.\n"
  "-d:      enable debug output.\n"
  "-t:      enable trace output.\n"
  "-p:      switch to pagetable tests.\n"
  "-nX:     set number of runs to X (default: 10000)\n";

void display_help_and_quit(void) {
  printf("%s", HELPTEXT);
  abort();
}