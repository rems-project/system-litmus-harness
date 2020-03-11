#include "lib.h"

void debug(const char* fmt, ...);

void debug_show_valloc_mem(void);
void debug_vmm_show_walk(uint64_t* pgtable, uint64_t va);