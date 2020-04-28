#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  uint64_t* x = data->var[0];
  uint64_t* z = data->var[2];
  uint64_t* zpte = data->PTE[2];
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, #1\n\t"
    "mov x4, %[y]\n\t"
    /* test payload */
    "str x0,[x1]\n\t"
    "dc civac,x1\n\t"
    "dsb sy\n\t"
    "tlbi vmalle1\n\t"
    "dsb sy\n\t"
    "str x3,[x4]\n\t"
  :
  : [zdesc] "r" (data->DESC[2]), [xpte] "r" (data->PTE[0]), [y] "r" (data->var[1])
  : "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
      /* move from C vars into machine regs */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"
      /* test */
      "ldr x0,[x1]\n\t"
      "dsb sy\n\t"
      "isb\n\t"
      "ldr x2,[x3]\n\t"
      /* output */
      "str x0, [%[outp1r0]]\n\t"
      "str x2, [%[outp1r2]]\n\t"
      :
      : [y] "r" (data->var[1]), [x] "r" (data->var[0]), [outp1r0] "r" (data->out_reg[0]), [outp1r2] "r" (data->out_reg[1])
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4");
}


litmus_test_t MPRT1_dcdsbtlbialldsb_dsbisb = {
  "MP.RT1+dc-dsb-tlbiall-dsb+dsb-isb",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  3,(const char*[]){"x", "y", "z"},
  2,(const char*[]){"p1:x0", "p1:x2"},
  .interesting_result = (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
  .start_els=(int[]){1,0},
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"z", TYPE_HEAP, 1},
  },
  .requires_pgtable=1,
};
