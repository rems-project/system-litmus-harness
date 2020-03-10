#include <stdint.h>

#include "lib.h"

static void* fault_handler(uint64_t esr, regvals_t* regs) {
  *(uint64_t*)regs->x4 = 1;

  /* return to next instruction in test */
  write_sysreg(read_sysreg(elr_el1)+4, elr_el1);
  return NULL;
}


static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {

  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* x2 = out_regs[0];
  uint64_t* x4 = out_regs[1];

  uint64_t* xpte = ptes[0];
  uint64_t* ypte = ptes[1];

  set_pgfault_handler((uint64_t)x, &fault_handler);

  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, %[ydesc]\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x2, #0\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, %[x4]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "ldr x2, [x3]\n\t"

      /* output */
      "mov %[x2], x2\n\t"
      : [x2] "=r" (*x2)
      : [ydesc] "r" (*ypte), [xpte] "r" (xpte), [x] "r" (x), [x4] "r" (x4)
      : "cc", "memory", "x0", "x1", "x2");

  reset_pgfault_handler((uint64_t)x);
}

void W_AT(void) {
  run_test("W+AT",
    1, (th_f** []){
      (th_f* []) {NULL, P0, NULL},
    },
    2, (const char* []){"x", "y"},
    2, (const char* []){"p0:x2", "p0:x3"},
    (test_config_t){
        .interesting_result = (uint64_t[]){
            /* p1:x2 =*/0,
            /* p1:x3 =*/1,
        },
        .no_init_states=1,
        .init_states=(init_varstate_t[]){
          (init_varstate_t){"x", TYPE_PTE, 0},
        }
    });
}
