#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"

    /* test */
    "str x0, [x1]\n\t"
  :
  : [zdesc] "r" (var_desc(data, "z")), [xpte] "r" (var_pte(data, "x"))
  : "cc", "memory", "x0", "x1"
  );
}

static void sync_handler_1(void) {
  asm volatile (
    "mov x0, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[y]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"

    /* output */
  :
  : [x] "r" (var_va(data, "x")), [y] "r" (var_va(data, "y")), [outp1r0] "r" (out_reg(data, "p1:x0"))
  : "cc", "memory", "x0", "x1", "x2", "x3", "x10", "x11"
  );
}

static void sync_handler_2(void) {
  asm volatile (
    "mov x2, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    "ldr x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"

    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
  :
  : [y] "r" (var_va(data, "y")), [x] "r" (var_va(data, "x")), [outp2r0] "r" (out_reg(data, "p2:x0")), [outp2r2] "r" (out_reg(data, "p2:x2"))
  : "cc", "memory", "x0", "x1", "x2", "x3", "x10", "x11"
  );
}


litmus_test_t WRCtrtinv_po_dmb = {
  "WRC.TRT.inv+po+dmb",
  3,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
    (th_f*)P2
  },
  3,(const char*[]){"x", "y", "z"},
  3,(const char*[]){"p1:x0", "p2:x0", "p2:x2"},
  .no_init_states=2,
  .init_states=(init_varstate_t*[]){
      &(init_varstate_t){"x", TYPE_PTE, 0},
      &(init_varstate_t){"z", TYPE_HEAP, 1},
    },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler_1, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler_2, NULL},
    },
  .interesting_result =
    (uint64_t[]){
      /* p1:x2 =*/ 1,
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 0,
    },
  .requires_pgtable=1,
  .no_sc_results = 7,
};