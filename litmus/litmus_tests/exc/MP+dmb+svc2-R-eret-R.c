#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile(
      "mov x0, #1\n\t"
      "str x0, [%[x1]]\n\t"
      "dmb sy\n\t"
      "mov x2, #1\n\t"
      "str x2, [%[x3]]\n\t"
      :
      : [x1] "r" (data->var[0]), [x3] "r" (data->var[1])
      :  "cc", "memory", "x0", "x2"
  );
}

static void svc_handler0(void) {
  asm volatile(
                           // Save registers
    "mrs x10,spsr_el1\n\t" // saved program status
    "mrs x11,elr_el1\n\t"  // return location
    "svc #0\n\t"
    "ldr x2, [x3]\n\t"
    "msr elr_el1,x11\n\t"  // restore
    "msr spsr_el1,x10\n\t"
    "eret\n\t");  
}

static void svc_handler1(void) {
  asm volatile(
      /* x1 = Y */
      "ldr x0, [x1]\n\t"
      "eret\n\t");
}

static void P1(litmus_test_run* data) {
  asm volatile(
               // TODO: update this
      /* load variables into machine registers */
      "mov x1, %[x1]\n\t"
      "mov x3, %[x3]\n\t"
      "svc #0\n\t"
      /* extract values */
      "str x0, [%[x0]]\n\t"
      "str x2, [%[x2]]\n\t"
      "dmb st\n\t"
      :
      : [x1] "r" (data->var[1])
      , [x3] "r" (data->var[0])
      , [x0] "r" (data->out_reg[0])
      , [x2] "r" (data->out_reg[1])
      : "cc", "memory",
        /* dont touch parameter registers */
        "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7", "x10", "x11" 
  );
}


litmus_test_t MP_dmb_svc2ReretR = {
  "MP+dmb+svc2-R-eret-R",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p1:x0", "p1:x2"},
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
