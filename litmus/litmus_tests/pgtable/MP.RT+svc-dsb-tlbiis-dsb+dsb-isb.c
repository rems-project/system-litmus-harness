#include <stdint.h>

#include "lib.h"

#define VARS x, y, z
#define REGS p1x0, p1x2

static void svc_handler(void) {
  asm volatile (
    "str x0,[x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1is,x2\n\t"
    "dsb sy\n\t"
    "str x3,[x4]\n\t"

    "eret\n\t"
  );
}

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x3, #1\n\t"
    "mov x4, %[y]\n\t"
    /* test payload */
    "svc #0\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P1(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
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
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}




litmus_test_t MPRT_svcdsbtlbiisdsb_dsbisb = {
  "MP.RT+svc-dsb-tlbiis-dsb+dsb-isb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0),
    INIT_VAR(z, 1)
  ),
  .interesting_result = (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)svc_handler, NULL},
     (uint32_t*[]){NULL, NULL},
  },
  .requires_pgtable=1,
  .no_sc_results = 3,
};