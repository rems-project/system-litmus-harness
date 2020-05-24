#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "dmb sy\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
  :
  : [x] "r" (var_va(data, "x")), [y] "r" (var_va(data, "y"))
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[y]]\n\t"
    "dmb sy\n\t"
    "ldr %[x2], [%[x]]\n\t"
  : [x0] "=&r" (*data->out_reg[0]), [x2] "=&r" (*data->out_reg[1])
  : [y] "r" (var_va(data, "y")), [x] "r" (var_va(data, "x"))
  : "cc", "memory"
  );
}

litmus_test_t MP_dmbs = {
  "MP+dmbs",
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