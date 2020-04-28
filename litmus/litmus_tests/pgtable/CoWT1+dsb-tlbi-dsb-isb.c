#include <stdint.h>

#include "lib.h"

static void svc_handler(void) {
  asm volatile (
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1, x4\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"
    "eret\n\t"
  );
}

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, %[ydesc]\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, %[xpage]\n\t"
      /* test */
      "svc #0\n\t"
      /* output back to C vars */
      "str x2, [%[x2]]\n\t"
      :
      : [ydesc] "r" (data->desc[1]), [xpte] "r" (data->pte[0]), [x] "r" (data->var[0]), [x2] "r" (data->out_reg[0]), [xpage] "r" (PAGE(data->var[0]))
      :  "cc", "memory", "x0", "x1", "x2", "x3"
  );
}



litmus_test_t CoWT1_dsbtlbidsbisb = {
  "CoWT1+dsb-tlbi-dsb-isb",
  1,(th_f*[]){
    (th_f*)P0
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"p0:x2",},
  .interesting_result = (uint64_t[]){
      /* p0:x2 =*/1,
  },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)svc_handler, NULL},
    },
  .no_init_states=2,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"x", TYPE_HEAP, 1},
    &(init_varstate_t){"y", TYPE_HEAP, 2},
  },
  .requires_pgtable=1,
};