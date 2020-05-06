#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile (
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x4, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1, x2\n\t"
    "dsb sy\n\t"
    "ldr x3, [x4]\n\t"

    /* output back to C vars */
    "str x3, [%[outx3]]\n\t"
    :
    : [ydesc] "r" (data->desc[1]), [xpte] "r" (data->pte[0]), [x] "r" (data->var[0]), [outx3] "r" (data->out_reg[0]), [xpage] "r" (PAGE(data->var[0]))
    :  "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

litmus_test_t CoWT1_dsbtlbidsb = {
  "CoWT1+dsb-tlbi-dsb",
  1,(th_f*[]){
    (th_f*)P0
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"p0:x3",},
  .interesting_result = (uint64_t[]){
      /* p0:x3 =*/0,
  },
  .start_els=(int[]){1,},
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"y", TYPE_HEAP, 1},
  },
  .requires_pgtable=1,
};