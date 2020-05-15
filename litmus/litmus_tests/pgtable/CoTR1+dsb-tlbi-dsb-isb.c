#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
    :
    : [ydesc] "r" (var_desc(data, "y")), [xpte] "r" (var_pte(data, "x"))
    : "cc", "memory", "x0", "x1"
  );
}


static void P1(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x1, %[x]\n\t"
    "mov x3, %[xpte]\n\t"
    "mov x4, %[xpage]\n\t"
    /* test payload */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1,x4\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"
    /* save results */
    "str x0, [%[outp1r0]]\n\t"
    "eor x2, x2, %[ydesc]\n\t"
    "cbz x2, .after\n\t"
    "mov x2, #1\n\t"
    ".after:\n\t"
    "eor x2, x2, #1\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : [x] "r" (var_va(data, "x")), [xpte] "r" (var_pte(data, "x")), [ydesc] "r" (var_desc(data, "y")), [xpage] "r" (var_page(data, "x")), [outp1r0] "r" (out_reg(data, "p1:x0")), [outp1r2] "r" (out_reg(data, "p1:x2"))
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}


litmus_test_t CoTR1_dsbtlbidsbisb = {
  "CoTR1+dsb-tlbi-dsb-isb",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p1:x0","p1:x2"},
  .interesting_result = (uint64_t[]){
      /* p0:x0 =*/1,
      /* p0:x2 =*/0,
  },
  .start_els=(int[]){0,1},
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"y", TYPE_HEAP, 1},
  },
  .requires_pgtable = 1,
  .no_sc_results = 3,
};