#ifndef ABORT_H
#define ABORT_H

#include "exceptions.h"
#include "printer.h"

void abort(void);

#define fail(...) \
  do { \
    printf_with_fileloc("abort", 1, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    raise_to_el1(); \
    abort(); \
  } while (0)

#define fail_on(COND, ...) \
  if ((COND)) { \
    printf_with_fileloc("abort", 1, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    raise_to_el1(); \
    abort(); \
  }
#endif /* ABORT_H */