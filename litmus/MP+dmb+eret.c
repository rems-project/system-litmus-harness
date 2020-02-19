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

static void svc_handler(void) {
  asm volatile(
      /* x1 = Y */
      "ldr x0, [x1]\n\t"
      "eret\n\t");
}

static uint32_t* old_vtentry;
static void P1_pre(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  old_vtentry = hotswap_exception(0x400, (uint32_t*)&svc_handler);
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* x0 = out_regs[0];
  uint64_t* x2 = out_regs[1];

  asm volatile(
      /* load variables into machine registers */
      "mov x1, %[x1]\n\t"
      "mov x3, %[x3]\n\t"

      "svc #0\n\t"
      "ldr x2, [x3]\n\t"

      /* extract values */
      "str x0, [%[x0]]\n\t"
      "str x2, [%[x2]]\n\t"
      "dmb st\n\t"
      :
      : [x1] "r"(y), [x3] "r"(x), [x0] "r" (x0), [x2] "r"(x2)
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7" /* dont touch parameter registers */
  );
}

static void P1_post(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  restore_hotswapped_exception(0x400, old_vtentry);
}

void MP_dmb_eret(void) {
  run_test("MP+dmb+eret",
    2, (th_f** []){
      (th_f* []) {NULL, P0, NULL},
      (th_f* []) {P1_pre, P1, P1_post},
    },
    2, (const char* []){"x", "y"},
    2, (const char* []){"p1:x0", "p1:x2"},
    (test_config_t){
        .interesting_result = (uint64_t[]){
            /* p1:x0 =*/1,
            /* p1:x2 =*/0,
        },
    });
}
