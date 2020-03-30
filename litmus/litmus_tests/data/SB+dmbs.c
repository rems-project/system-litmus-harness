#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x2 = out_regs[0];

  asm volatile (
    /* load registers */
    "mov x1, %[x1]\n\t"
    "mov x3, %[x3]\n\t"

    "mov x0, #1\n\t"
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* collect results */
    "str x2, [%[x2]]\n\t"
  :
  : [x1] "r" (x), [x3] "r" (y), [x2] "r" (x2)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x2 = out_regs[1];

  asm volatile (
    /* load registers */
    "mov x1, %[x1]\n\t"
    "mov x3, %[x3]\n\t"

    "mov x0, #1\n\t"
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* collect results */
    "str x2, [%[x2]]\n\t"
  :
  : [x1] "r" (y), [x3] "r" (x), [x2] "r" (x2)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

void SB_dmbs(void) {
  run_test(
    "SB+dmbs",
    2, (th_f**[]){
      (th_f* []){NULL, P0, NULL},
      (th_f* []){NULL, P1, NULL},
    },
    2, (const char*[]){"x", "y"}, 
    2, (const char*[]){"p0:x2", "p1:x2"}, 
    (test_config_t){
      .interesting_result =
        (uint64_t[]){
          /* p1:x0 =*/ 0,
          /* p1:x2 =*/ 0,
        },
    });
}
