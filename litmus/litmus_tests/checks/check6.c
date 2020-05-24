#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x]]\n\t"
    "mov x2, #2\n\t"
    "str x2, [%[x]]\n\t"
  : [x0] "=&r" (*data->out_reg[0])
  : [x] "r" (var_va(data, "x"))
  : "cc", "memory"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr x0, [%[x]]\n\t"
    "cbz x0, .after\n\t"
    "str %[ydesc], [%[xpte]]\n\t"
    ".after:\n\t"
  :
  : [x] "r" (var_va(data, "x")), [ydesc] "r" (var_desc(data, "y")), [xpte] "r" (var_pte(data, "x"))
  : "cc", "memory", "x0"
  );
}

litmus_test_t check6 = {
  "check6",
  2, (th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"p0:x0"},
  .interesting_result =
    (uint64_t[]){
      /* p0:x0 =*/ 1,
    },
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"y", TYPE_HEAP, 1},
  },
};