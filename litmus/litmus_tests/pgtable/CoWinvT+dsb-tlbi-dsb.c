#include <stdint.h>

#include "lib.h"

static void* fault_handler(uint64_t esr, regvals_t* regs) {
  *(uint64_t*)regs->x4 = 1;

  /* return to next instruction in test */
  write_sysreg(read_sysreg(elr_el1)+4, elr_el1);
  return NULL;
}


static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {

  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* x4 = out_regs[0];

  uint64_t* xpte = ptes[0];
  uint64_t* ypte = ptes[1];

  uint64_t xpage = (uint64_t)x >> 12;

  set_pgfault_handler((uint64_t)x, &fault_handler);

  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, #0\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, %[x4]\n\t"
      "mov x5, %[xpage]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "dsb sy\n\t"
      "tlbi vae1,x5\n\t"
      "dsb sy\n\t"
      "ldr x2, [x3]\n\t"
      :
      : [ydesc] "r" (*ypte), [xpte] "r" (xpte), [x] "r" (x), [x4] "r" (x4), [xpage] "r" (xpage)
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5");

  reset_pgfault_handler((uint64_t)x);
}

litmus_test_t CoWinvT_dsbtlbidsb = {
  "CoWinvT+dsb-tlbi-dsb",
  1, (th_f** []){
    (th_f* []) {NULL, P0, NULL},
  },
  1, (const char* []){"x"},
  1, (const char* []){"p0:x4",},
  .start_els=(int[]){1,},
  .interesting_result = (uint64_t[]){
      /* p0:x4 =*/0,
  },
  .requires_pgtable = 1,
};
