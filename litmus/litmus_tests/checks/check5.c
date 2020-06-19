#include <stdint.h>

#include "lib.h"

#define VARS x
#define REGS p0x0

static void handler(void) {
  asm volatile (
    "mov x0, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr x0, [%[x]]\n\t"
    "mov x1, #0\n\t"
    "str x1, [%[xpte]]\n\t"
    "str x0, [%[outp0r0]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x10"
  );
}


litmus_test_t check5 = {
  "check5",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    1,
    INIT_VAR(x, 1)
  ),
  .interesting_result =
    (uint64_t[]){
      /* p0:x0 =*/ 0,
    },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)handler, NULL},
    },
  .requires_pgtable=1,
};