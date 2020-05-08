#include <stdint.h>

#include "lib.h"

static void sync_handler(void) {
  asm volatile (
    "mov x2, #1\n\t"

    ERET_TO_NEXT(x10)
  );
}


static void P0(litmus_test_run* data) {
  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, #0\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, %[xpage]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "dsb sy\n\t"
      "tlbi vae1,x4\n\t"
      "dsb sy\n\t"
      "ldr x2, [x3]\n\t"

      /* output */
      "str x2, [%[outp0r2]]\n\t"
      :
      : [xpte] "r" (data->pte[0]), [x] "r" (data->var[0]), [xpage] "r" (PAGE(data->var[0])), [outp0r2] "r" (data->out_reg[0])
      :  "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10"
  );
}


litmus_test_t CoWinvT1_dsbtlbidsb = {
  "CoWinvT1+dsb-tlbi-dsb",
  1,(th_f*[]){
    (th_f*)P0
  },
  1,(const char*[]){"x",},
  1,(const char*[]){"p0:x2",},
  .start_els=(int[]){1,},
  .interesting_result = (uint64_t[]){
      /* p0:x2 =*/0,
  },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, (uint32_t*)sync_handler},
  },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};