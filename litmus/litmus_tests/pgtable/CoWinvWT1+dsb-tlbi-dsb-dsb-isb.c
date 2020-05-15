#include <stdint.h>

#include "lib.h"

static void sync_handler(void) {
  asm volatile (
    "mov x5, #2\n\t"

    ERET_TO_NEXT(x10)
  );
}


static void P0(litmus_test_run* data) {
  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, #0\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x2, %[xpage]\n\t"
      "mov x3, %[ydesc]\n\t"
      "mov x4, %[xpte]\n\t"
      "mov x6, %[x]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "dsb sy\n\t"
      "tlbi vae1,x2\n\t"
      "dsb sy\n\t"
      "str x3, [x4]\n\t"
      "dsb sy\n\t"
      "isb\n\t"
      "ldr x5, [x6]\n\t"

      /* output */
      "str x5, [%[outp0r5]]\n\t"
      :
      : [xpte] "r" (var_pte(data, "x")), [xpage] "r" (var_page(data, "x")), [ydesc] "r" (var_desc(data, "y")), [x] "r" (var_va(data, "x")), [outp0r5] "r" (out_reg(data, "p0:x5"))
      :  "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10"
  );
}


litmus_test_t CoWinvWT1_dsbtlbidsbdsbisb = {
  "CoWinvWT1+dsb-tlbi-dsb-dsb-isb",
  1,(th_f*[]){
    (th_f*)P0
  },
  2,(const char*[]){"x","y",},
  1,(const char*[]){"p0:x5",},
  .no_interesting_results=2,
  .interesting_results = (uint64_t*[]){
    (uint64_t[]){
      /* p0:x5 =*/0,  /* stale translation */
    },
    (uint64_t[]){
      /* p0:x5 =*/2,  /* spurious abort */
    },
  },
  .start_els=(int[]){1,},
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, (uint32_t*)sync_handler},
  },
  .no_init_states=2,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"x", TYPE_HEAP, 0},
    &(init_varstate_t){"y", TYPE_HEAP, 1},
  },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};