#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "str x0, [%[x1]]\n\t"
    "dmb sy\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[x3]]\n\t"
  :
  : [x1] "r" (data->var[0]), [x3] "r" (data->var[1])
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x1]]\n\t"
    "cbz %[x0], .after\n\t"
    "str %[x2], [%[x3]]\n\t"
    ".after:\n\t"
  : [x0] "=&r" (*data->out_reg[0])
  : [x1] "r" (data->var[1]), [x2] "r" (data->out_reg[0]), [x3] "r" (data->pte[0])
  : "cc", "memory"
  );
}

litmus_test_t check3 = {
  .name="check3",
  .no_threads=2,
  .threads=(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },

  .no_heap_vars=2,
  .heap_var_names=(const char*[]){"x", "y"},

  .no_regs=2,
  .reg_names=(const char*[]){"p1:x0", "p1:x2"},

  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p1:x2 =*/ 0,
    },

  .requires_pgtable = 1,
};