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
  :  [x] "r" (data->var[0]), [yprimedesc] "r" (*yprimepte), [ypte] "r" (data->pte[0])
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
  : [y] "r" (data->var[1]), [z] "r" (data->var[2]), [outp1r0] "r" (data->out_reg[0])
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
    "str x0, [%[p2r0]]\n\t"
    "str x2, [%[p2r2]]\n\t"
  :
  : [z] "r" (data->var[2]), [x] "r" (data->var[0]), [p2r0] "r" (data->out_reg[1]), [p2r2] "r" (data->out_reg[2])
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
};