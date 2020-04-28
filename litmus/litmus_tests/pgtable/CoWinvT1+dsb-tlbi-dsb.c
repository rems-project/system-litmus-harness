#include <stdint.h>

#include "lib.h"

static void* fault_handler(uint64_t esr, regvals_t* regs) {
  *(uint64_t*)regs->x4 = 1;

  /* return to next instruction in test */
  write_sysreg(read_sysreg(elr_el1)+4, elr_el1);
  return NULL;
}


static void P0(litmus_test_run* data) {
  uint64_t* y = data->var[1];
  uint64_t* ypte = data->PTE[1];
  set_pgfault_handler((uint64_t)data->var[0], &fault_handler);
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
      : [ydesc] "r" (data->DESC[1]), [xpte] "r" (data->PTE[0]), [x] "r" (data->var[0]), [x4] "r" (data->out_reg[0]), [xpage] "r" (PAGE(data->var[0]))
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5");
  reset_pgfault_handler((uint64_t)data->var[0]);
}


litmus_test_t CoWinvT1_dsbtlbidsb = {
  "CoWinvT1+dsb-tlbi-dsb",
  1,(th_f*[]){
    (th_f*)P0
  },
  1,(const char*[]){"x"},
  1,(const char*[]){"p0:x4",},
  .start_els=(int[]){1,},
  .interesting_result = (uint64_t[]){
      /* p0:x4 =*/0,
  },
  .requires_pgtable = 1,
};
