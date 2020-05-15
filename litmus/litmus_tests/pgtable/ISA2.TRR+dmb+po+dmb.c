#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  uint64_t* yprimepte = data->pte[3];
  asm volatile (
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    "mov x2, %[yprimedesc]\n\t"
    "mov x3, %[ypte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "str x2, [x3]\n\t"
  :
  :  [x] "r" (var_va(data, "x")), [yprimedesc] "r" (*yprimepte), [ypte] "r" (var_pte(data, "x"))
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[z]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"
    /* output */
    "str x0, [%[outp1r0]]\n\t"
  :
  : [y] "r" (var_va(data, "y")), [z] "r" (var_va(data, "z")), [outp1r0] "r" (out_reg(data, "p1:x0"))
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[z]\n\t"
    "mov x3, %[x]\n\t"
    "ldr x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"
    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
  :
  : [z] "r" (var_va(data, "z")), [x] "r" (var_va(data, "x")), [outp2r0] "r" (out_reg(data, "p2:x0")), [outp2r2] "r" (out_reg(data, "p2:x2"))
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}



litmus_test_t ISA2trr_dmb_po_dmb = {
  "ISA2.TRR+dmb+po+dmb",
  3,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
    (th_f*)P2
  },
  4,(const char*[]){"x", "y", "z", "y'"},
  3,(const char*[]){"p1:x0", "p2:x0", "p2:x2"},
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
      &(init_varstate_t){"y'", TYPE_HEAP, 1},
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