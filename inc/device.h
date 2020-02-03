#include <stdint.h>

void init_device(void);

/* top of physical memory */
uint64_t TOP_OF_MEM;

/* total allocated heap space */
uint64_t TOTAL_HEAP;

/* bottom of heap space */
uint64_t BOT_OF_HEAP;

/**
 * Top and Bottom of thread stacks
 */
uint64_t TOP_OF_STACK;
uint64_t BOT_OF_STACK;

/**
 * Sections from linker
 */
extern unsigned long text_end; /* end of .text section (see bin.lds) */
extern unsigned long data_end; /* end of .bss and .rodata section */
extern unsigned long stacktop; /* end of .bss and .rodata section */

uint64_t TOP_OF_TEXT;
uint64_t TOP_OF_DATA;