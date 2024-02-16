#include <stdint.h>

#include "lib.h"

#define VARS x
#define REGS p1x0

static void P0(litmus_test_run* data) {
  asm volatile (
      "mov x0, #0\n\t"
      "str x0, [%[x]]\n\t"

      /* out */
      "str x0, [%[outp1r0]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x10"
  );
}

static void sync_handler(void) {
  asm volatile(
      /* if EC==100100 (DataAbort) */
      "mrs x0, esr_el1\n"
      "ubfx x0, x0, #26, #6\n"
      "cmp x0, #0b100100\n"
      "b.ne 0f\n"

      /* if ISS[6:0]==1010000 (WnR==1 , DFSC==01000
                               Write-induced, Sync SError) */
      "mrs x0, esr_el1\n"
      "ubfx x0, x0, #0, #7\n"
      "cmp x0, #0b1010000\n"
      "b.ne 0f\n"

      /* return 1 */
      "1: mov x0, #1\n"
      ERET_TO_NEXT(x10)

      /* else: return 0 */
      "0: mov x0, #0\n"
      ERET_TO_NEXT(x10)
  );
}

litmus_test_t SEA_W_detect = {
  "SEA_W_detect",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    1,
    /* fixed address out-of-range of any SDRAM/physical device */
    /* TODO: maybe add an INIT_UNBACKED() that reads DTB so test becomes portable? */
    INIT_FIX(x, 0x800000000ULL)
  ),
  .thread_sync_handlers =
    (u32**[]){
     (u32*[]){(u32*)sync_handler, NULL},
    },
  .interesting_result = (uint64_t[]){
      /* if p1:x0 was 1, then saw a Synchronous SError on the Load
       * hence SEA_R must be True.
       */
      /* p1:x0 =*/1,
  },
};
