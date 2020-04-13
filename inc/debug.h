#include "lib.h"

#define debug(...) _debug(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

void _debug(const char* file, const int line, const char* func, const char* fmt, ...);

void debug_show_valloc_mem(void);
void debug_vmm_show_walk(uint64_t* pgtable, uint64_t va);
void debug_valloc_status(void);