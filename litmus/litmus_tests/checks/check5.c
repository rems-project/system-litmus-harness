#include <stdint.h>

#include "lib.h"

static void handler(void) {
  asm volatile (
    "mov x0, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr x0, [%[x]]\n\t"
    "str %[invl], [%[xpte]]\n\t"
    "str x0, [%[outp0r0]]\n\t"
  :
  : [x] "r" (var_va(data, "x")), [invl] "r" (0), [xpte] "r" (var_pte(data, "x")), [outp0r0] "r" (out_reg(data, "p0:x0"))
  : "cc", "memory", "x0", "x10"
  );
}

litmus_test_t check5 = {
  "check5",
  1, (th_f*[]){
    (th_f*)P0,
  },
  1,(const char*[]){"x"},
  1,(const char*[]){"p0:x0"},
  .interesting_result =
    (uint64_t[]){
      /* p0:x0 =*/ 0,
    },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)handler, NULL},
    },
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"x", TYPE_HEAP, 1},
  },
  .requires_pgtable=1,
};