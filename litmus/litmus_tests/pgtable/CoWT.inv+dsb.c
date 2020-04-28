#include <stdint.h>

#include "lib.h"

static void* fault_handler(uint64_t esr, regvals_t* regs) {
  *(uint64_t*)regs->x4 = 1;

  /* return to next instruction in test */
  write_sysreg(read_sysreg(elr_el1)+4, elr_el1);
  return NULL;
}


static void P0(litmus_test_run* data) {
  set_pgfault_handler((uint64_t)data->var[0], &fault_handler);
  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, %[ydesc]\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, %[x4]\n\t"
      /* test */
      "str x0, [x1]\n\t"
      "dsb sy\n\t"
      "ldr x2, [x3]\n\t"
      :
      : [ydesc] "r" (data->desc[1]), [xpte] "r" (data->pte[0]), [x] "r" (data->var[0]), [x4] "r" (data->out_reg[0])
      :  "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
  reset_pgfault_handler((uint64_t)data->var[0]);
}


litmus_test_t CoWTinv_dsb = {
  "CoWT.inv+dsb",
  1,(th_f*[]){
    (th_f*)P0
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"p0:x4",},
  .interesting_result = (uint64_t[]){
      /* p0:x4 =*/1,
  },
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"x", TYPE_PTE, 0},
  },
  .requires_pgtable=1,
};