#include <stdint.h>

#include "lib.h"

char __copy[1024];

/* these must be 64-bit aligned */
char*  __argv[1024];
uint64_t    __argc;

void init_args(void) {
    char* bootargs = dtb_bootargs(fdt);

    /* make copy */
    valloc_memcpy(__copy, bootargs, strlen(bootargs));
    bootargs = __copy;

    char* word = bootargs;
    __argc = 0;

    for (int i = 0; i < 1024; i++) {
        /* no more words */
        if (*bootargs == '\0') {
          break;
        }

        /* scan to begining of next word */
        while (*bootargs && *bootargs != ' ')  {
            bootargs++;
        }

        if (*bootargs == ' ') {
          *bootargs = '\0';
          bootargs++;
        }

        /* record this word */
        __argv[__argc] = word;
        __argc++;

        word = bootargs;
    }
}