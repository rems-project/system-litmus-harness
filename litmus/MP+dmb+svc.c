#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x0 = out_regs[0];
  uint64_t* x2 = out_regs[1];

  asm volatile(
      "mov x0, #1\n\t"
      "str x0, [%[x1]]\n\t"
      "dmb sy\n\t"
      "mov x2, #1\n\t"
      "str x2, [%[x3]]\n\t"
      :
      : [x1] "r"(x), [x3] "r"(y)
      : "cc", "memory", "x0", "x2");
}

static void svc_handler(uint64_t esr, regvals_t* regs) {
  /* invariant: only called within an SVC */
  uint64_t* x = (uint64_t*)regs->x0;
  uint64_t* x2 = (uint64_t*)regs->x1;

  asm volatile("ldr %[x2], [%[x3]]\n"
               : [x2] "=&r"(*x2)
               : [x3] "r"(x)
               : "memory");
}

__attribute__((inline)) static void INLINED_svc_handler(void) {
  asm volatile(
      /* x0 = X */
      "ldr x2, [x0]\n\t"
      "eret\n\t");
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x0 = out_regs[0];
  uint64_t* x2 = out_regs[1];

  // set_svc_handler(0, svc_handler);
  uint32_t* old_entry = hotswap_exception(0x200, &INLINED_svc_handler);

  asm volatile(
      /* arguments passed to SVC through x0,x1..x7 */
      "mov x0, %[x2]\n\t" /* pointer to x */

      "ldr %[x0], [%[x1]]\n\t"
      "svc #0\n\t"

      /* extract value */
      "str x2, [%[x3]]\n\t"
      : [x0] "=&r"(*x0)
      : [x1] "r"(y), [x2] "r"(x), [x3] "r"(x2)
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7" /* dont touch parameter registers */
  );

  // reset_svc_handler(0);
  restore_hotswapped_exception(0x200, old_entry);
}

void MyMP_dmb_svc(void) {
  run_test("MP+dmb+svc", 2, (th_f* []){P0, P1}, 2, (const char* []){"x", "y"},
           2, (const char* []){"p1:x0", "p1:x2"},
           (test_config_t){
               .interesting_result = (uint64_t[]){
                   /* p1:x0 =*/1,
                   /* p1:x2 =*/0,
               },
           });
}
