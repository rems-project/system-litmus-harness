#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    /* test */
    "str x0, [x1]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1])
  : "cc", "memory", "x0", "x1"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[y]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "str x2, [x4]\n\t"
    "str x0, [%[outp1r0]]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [outp1r0] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "ldr x2, [x4]\n\t"
    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [outp2r0] "r" (data->out_reg[1]), [outp2r2] "r" (data->out_reg[2])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}


litmus_test_t WRC_addrs = {
  "WRC+addrs",
  3,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
    (th_f*)P2
  },
  2,(const char*[]){"x", "y"},
  3,(const char*[]){"p1:x0", "p2:x0", "p2:x2"},
  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 0,
    },
};