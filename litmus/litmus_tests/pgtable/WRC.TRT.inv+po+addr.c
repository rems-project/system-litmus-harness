#include <stdint.h>

#include "lib.h"

#define VARS x, y, z
#define REGS p1x0, p2x0, p2x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"

    /* test */
    "str x0, [x1]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1"
  );
}

static void sync_handler_1(void) {
  asm volatile (
    "mov x0, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[y]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"

    /* output */
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x10", "x11"
  );
}

static void sync_handler_2(void) {
  asm volatile (
    "mov x2, #0\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "ldr x2, [x3, x4]\n\t"

    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10"
  );
}



litmus_test_t WRCtrtinv_po_addr = {
  "WRC.TRT.inv+po+addr",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3,
    INIT_UNMAPPED(x),
    INIT_VAR(y, 0),
    INIT_VAR(z, 1)
  ),
   .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler_1, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler_2, NULL},
    },
  .interesting_result =
    (uint64_t[]){
      /* p1:x2 =*/ 1,
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 0,
    },
  .requires_pgtable=1,
  .no_sc_results = 7,
};