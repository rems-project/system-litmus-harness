#ifndef ABORT_H
#define ABORT_H

#include "exceptions.h"
#include "printer.h"

void abort(void);

#define fail(...)                                                                                                        \
  do {                                                                                                                   \
    current_thread_info()->locking_enabled = false;                                                                      \
    printf("\n");                                                                                                        \
    printf_with_fileloc(                                                                                                 \
      "abort", PRINT_MODE_ERROR | PRINT_MODE_WARNING | PRINT_MODE_TRACE, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__ \
    );                                                                                                                   \
    raise_to_el1();                                                                                                      \
    abort();                                                                                                             \
    __builtin_unreachable();                                                                                             \
  } while (0)

#define fail_on(COND, ...)                                                                                               \
  if ((COND)) {                                                                                                          \
    current_thread_info()->locking_enabled = false;                                                                      \
    printf("\n");                                                                                                        \
    printf_with_fileloc(                                                                                                 \
      "abort", PRINT_MODE_ERROR | PRINT_MODE_WARNING | PRINT_MODE_TRACE, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__ \
    );                                                                                                                   \
    raise_to_el1();                                                                                                      \
    abort();                                                                                                             \
    __builtin_unreachable();                                                                                             \
  }
#endif /* ABORT_H */