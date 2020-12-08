#ifndef DEBUG_H
#define DEBUG_H
#include <stdint.h>

#include "debugging/debug_flags.h"
#include "debugging/tostr.h"


#define debug(...) \
  do { \
    if (DEBUG) {\
      printf_with_fileloc("DEBUG", 1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
    } \
  } while (0)


#define DEBUG(flag, ...) \
  do { \
    if (DEBUG && flag) { \
      printf_with_fileloc(#flag, 1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
    } \
  } while (0)


void debug_show_valloc_mem(void);
void debug_vmm_show_walk(uint64_t* pgtable, uint64_t va);
void debug_valloc_status(void);

void dump_hex(char* dest, char* src, int len);
#endif /* DEBUG_H */