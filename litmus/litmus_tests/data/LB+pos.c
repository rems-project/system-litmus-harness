#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* outp0r2 = out_regs[0];

  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[y]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"

    "str x0, [%[outp0r2]]\n\t"
  :
  : [x] "r" (x), [y] "r" (y), [outp0r2] "r" (outp0r2)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* outp1r2 = out_regs[0];

  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"

    "str x0, [%[outp1r2]]\n\t"
  :
  : [x] "r" (x), [y] "r" (y), [outp1r2] "r" (outp1r2)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}


litmus_test_t LB_pos = {
  "LB+pos",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p0:x2", "p1:x2"},
  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p1:x2 =*/ 0,
    },
};