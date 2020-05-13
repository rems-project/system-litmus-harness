#include <stdint.h>

#include "lib.h"

static void svc_handler0(void) {
  asm volatile(
      /* x3 = Y */
      "ldr x2, [x3]\n\t"
      "eret\n\t");
}


static void P0(litmus_test_run* data) {
  asm volatile(
      /* initial registers */
      "mov x0, #1\n\t"
      "mov x1, %[x]\n\t"
      "mov x3, %[y]\n\t"
               
      "str x0, [x1]\n\t"
      "svc #0\n\t"
      /* extract values */
      "str x2, [%[outp0r2]]\n\t"
      "dmb st\n\t"
      :
      : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [outp0r2] "r" (data->out_reg[0])
      :  "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void svc_handler1(void) {
  asm volatile(
      /* x3 = X */
      "ldr x2, [x3]\n\t"
      "eret\n\t");
}

static void P1(litmus_test_run* data) {
  asm volatile(
      /* initial registers */
      "mov x0, #1\n\t"
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"
      
      "str x0, [x1]\n\t"
      "svc #0\n\t"
      "str x2, [%[outp1r2]]\n\t"
      "dmb st\n\t"
      :
      : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [outp1r2] "r" (data->out_reg[1])
      : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7" /* dont touch parameter registers */
  );
}


litmus_test_t SB_WsvcR_WsvcR = {
  "SB+W-svc-R+W-svc-R",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p0:x2", "p1:x2"},
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)svc_handler0, NULL},
     (uint32_t*[]){(uint32_t*)svc_handler1, NULL},
    },
  .interesting_result = (uint64_t[]){
      /* p0:x2 =*/0,
      /* p1:x2 =*/0,
  },
};
