#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "str x0, [%[x1]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[x3]]\n\t"
  :
  : [x1] "r" (var_va(data, "x")), [x3] "r" (var_va(data, "y"))
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x1]]\n\t"
    "ldr %[x2], [%[x3]]\n\t"
  : [x0] "=&r" (*data->out_reg[0]), [x2] "=&r" (*data->out_reg[1])
  : [x1] "r" (var_va(data, "y")), [x3] "r" (var_va(data, "x"))
  : "cc", "memory"
  );
}


litmus_test_t MP_pos = {
  "MP+pos",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p1:x0", "p1:x2"},
  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p1:x2 =*/ 0,
  },
  .no_sc_results = 3,
};