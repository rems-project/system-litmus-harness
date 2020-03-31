#include <stdint.h>

#include "lib.h"

static void svc_handler(void) {
  asm volatile (
    "str x0,[x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1,x2\n\t"
    "dsb sy\n\t"
    "str x3,[x4]\n\t"

    "eret\n\t"
  );
}

static uint32_t* old_vtentry;
static void P0_pre(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  old_vtentry = hotswap_exception(0x400, (uint32_t*)&svc_handler);
}


static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {

  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* z = heap_vars[2];

  uint64_t* xpte = ptes[0];
  uint64_t* zpte = ptes[2];

  uint64_t zdesc = *zpte;
  uint64_t xpage = (uint64_t)x >> 12;

  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x3, #1\n\t"
    "mov x4, %[y]\n\t"

    /* test payload */
    "svc #0\n\t"

  :
  : [zdesc] "r" (zdesc), [xpte] "r" (xpte), [xpage] "r" (xpage), [y] "r" (y)
  : "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P0_post(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  restore_hotswapped_exception(0x400, old_vtentry);
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {

  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* outp1r0 = out_regs[0];
  uint64_t* outp1r2 = out_regs[1];

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
      : [y] "r" (y), [x] "r" (x), [outp1r0] "r" (outp1r0), [outp1r2] "r" (outp1r2)
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4");
}

litmus_test_t MPRT_svcdsbtlbidsb_dsbisb = {
  "MP.RT+svc-dsb-tlbi-dsb+dsb-isb",
  2, (th_f** []){
    (th_f* []) {P0_pre, P0, P0_post},
    (th_f* []) {NULL, P1, NULL},
  },
  3, (const char* []){"x", "y", "z"},
  2, (const char* []){"p1:x0", "p1:x2"},
  .interesting_result = (uint64_t[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"z", TYPE_HEAP, 1},
  },
  .requires_pgtable=1,
};
