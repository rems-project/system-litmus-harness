#include <stdint.h>

#include "lib.h"

#define VARS x, y, z
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #0\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[zdesc]\n\t"
    "mov x4, %[xpte]\n\t"
    "mov x5, #1\n\t"
    "mov x6, %[y]\n\t"
    /* test payload */
    "str x0,[x1]\n\t"
    "dmb sy\n\t"
    "str x3,[x4]\n\t"
    "dmb sy\n\t"
    "str x5,[x6]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6"
  );
}

static void sync_handler(void) {
  asm volatile(
    "mov x2, #2\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"
      /* test */
      "ldr x0,[x1]\n\t"
      "dmb sy\n\t"
      "ldr x2,[x3]\n\t"
      /* output */
      "str x0, [%[outp1r0]]\n\t"
      "str x2, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x10"
  );
}



litmus_test_t MPBBM_dmbdmb_dmb = {
  "MP.BBM+dmb-dmb+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0),
    INIT_VAR(z, 1)
  ),
  .no_interesting_results=2,
  .interesting_results = (uint64_t*[]){
    (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,  /* stale translation */
    },
    (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/2,  /* spurious abort */
    },
  },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler, NULL},
    },
  .requires_pgtable=1,
  .no_sc_results = 4,
};