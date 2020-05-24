#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
      "str x0, [%[x]]\n\t"
      "dmb sy\n\t"
      "mov x2, #1\n\t"
      "str x2, [%[y]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}

static void svc_handler0(void) {
  asm volatile(
                           // Save registers
    "mrs x10,spsr_el1\n\t" // saved program status
    "mrs x11,elr_el1\n\t"  // return location
     /* x1 = Y */
    "ldr x0, [x1]\n\t"
    "svc #0\n\t"
    "msr elr_el1,x11\n\t"  // restore
    "msr spsr_el1,x10\n\t"
    "eret\n\t");
}

static void svc_handler1(void) {
  asm volatile(
      "ldr x2, [x3]\n\t"
      "eret\n\t");
}

static void P1(litmus_test_run* data) {
  asm volatile (
    // TODO: update this
      /* load variables into machine registers */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"
      "svc #0\n\t"
      /* extract values */
      "str x0, [%[outp1r0]]\n\t"
      "str x2, [%[outp1r2]]\n\t"
      "dmb st\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory",
        /* dont touch parameter registers */
        "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7", "x10", "x11"
  );
}



litmus_test_t MP_dmb_svcRsvcR = {
  "MP+dmb+svc-R-svc-R",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, NULL},
     (uint32_t*[]){(uint32_t*)svc_handler0, (uint32_t*)svc_handler1},
    },
  .interesting_result = (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
};
