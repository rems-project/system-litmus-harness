#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p0x2

static void sync_handler(void) {
  asm volatile (
    "mov x2, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P0(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
      "mov x0, %[ydesc]\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "ldr x2, [x3]\n\t"

      /* output */
      "str x2, [%[outp0r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x10"
  );
}


litmus_test_t CoWTinv = {
  "CoWT.inv",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_UNMAPPED(x),
    INIT_VAR(y, 1)
  ),
  .interesting_result = (uint64_t[]){
    /* p0:x2 =*/0,
  },
  .thread_sync_handlers = (uint32_t**[]){
    (uint32_t*[]){(uint32_t*)sync_handler, NULL},
  },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};