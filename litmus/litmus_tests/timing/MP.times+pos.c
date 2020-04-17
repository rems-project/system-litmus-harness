#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* out_time_str0 = out_regs[2];
  uint64_t* out_time_str1 = out_regs[3];

  printf("pmccntr = %p\n", read_sysreg(pmccntr_el0));

  asm volatile (
    "mov x0, #1\n\t"
    "mrs x5, pmccntr_el0\n\t"
    "str x0, [%[x1]]\n\t"
    "mrs x6, pmccntr_el0\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[x3]]\n\t"
    "mrs x7, pmccntr_el0\n\t"

    "sub x7, x6, x7\n\t"
    "sub x6, x5, x6\n\t"
    
    "str x6, [%[tstr0]]\n\t"
    "str x7, [%[tstr1]]\n\t"
  :
  : [x1] "r" (x), [x3] "r" (y), [tstr0] "r" (out_time_str0), [tstr1] "r" (out_time_str1)
  : "cc", "memory", "x0", "x2", "x5", "x6", "x7"
  );
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x0 = out_regs[0];
  uint64_t* x2 = out_regs[1];

  uint64_t* out_time_ldr0 = out_regs[4];
  uint64_t* out_time_ldr1 = out_regs[5];

  asm volatile (
    "mrs x5, pmccntr_el0\n\t"
    "ldr %[x0], [%[x1]]\n\t"
    "mrs x6, pmccntr_el0\n\t"
    "ldr %[x2], [%[x3]]\n\t"
    "mrs x7, pmccntr_el0\n\t"

    "sub x7, x6, x7\n\t"
    "sub x6, x5, x6\n\t"
    
    "str x6, [%[tldr0]]\n\t"
    "str x7, [%[tldr1]]\n\t"
  : [x0] "=&r" (*x0), [x2] "=&r" (*x2)
  : [x1] "r" (y), [x3] "r" (x), [tldr0] "r" (out_time_ldr0), [tldr1] "r" (out_time_ldr1)
  : "cc", "memory", "x5", "x6", "x7"
  );
}


litmus_test_t MPtimes_pos = {
  "MP.times+pos",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  6,(const char*[]){"p1:x0", "p1:x2", "time_str0", "time_str1", "time_ldr0", "time_ldr1"},
  .interesting_result = NULL,
  .requires_perf = 1,
};
