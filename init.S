.section .init

#define __ASSEMBLY__
#include "asm.h"

/* at the start of time, we boot into EL2 on a CPU
 * before we can do anything else, we have to setup the initial register state
 */

/* init_cpu initialises a CPU
 * clobbers x0-x9
 *          x10-x15
 *          x20
 */
.globl init_cpu
init_cpu:
  mov x20, x30
  /* x10 = Current CPU # */
  mrs x10, mpidr_el1
  and x10, x10, #0xff
  msr tpidr_el0, x10

  /* x11 = PSTATE.EL */
  mrs x11, CurrentEL
  lsr x11, x11, #2

  /* each (thread, EL) has a 2MiB region carved out
    * for its stack, with 3 contiguous 2MiB regions
    * per thread.
    *
    * the regions begin at END_OF_LINKER_SECTIONS (aka __ld_end_sections)
    * starting with (CPU0,EL0) and then goes up:
    *   +0MiB == ALIGN_UP(__ld_end_sections, 2*MiB)
    *   +0MiB (CPU0,EL0)
    *   +2MiB (CPU0,EL1)
    *   +4MiB (CPU0,EL2)
    *   +6MiB  ...
    *   +20MiB (CPU3,EL1)
    *   +22MiB (CPU3,EL2)
    */

  adrp x12, __ld_end_sections
  add x12, x12, :lo12:__ld_end_sections
  /* x13 = 2MiB = STACK_SIZE */
  mov x13, #1
  lsl x13,x13,#21
  /* align BOT_OF_STACK _up_ to 2M */
  add x12, x13, x12
  sub x15, x13, #1
  mvn x15, x15
  and x12, x12, x15

  /* x14 = BOT_OF_STACK_CPU */
  mov x15, x13
  mov x14, #3
  mul x15, x15, x14
  add x12, x12, x15

  mov x0, x10
  mov x1, x11
  mov x2, x12
  mov x3, x13
  bl _init_cpu_el1
  cmp x11, #2
  b.ne 1f
  mov x0, x10
  mov x1, x11
  mov x2, x12
  mov x3, x13
  bl _init_cpu_el2
1:
  mov x4, #0
  msr spsel, x4  /* and use SP_EL0 outside of exception handlers */
  isb
  mov x30, x20
  ret

/* perform initialisation of the EL1 state
 *
 * by doing the following:
 *   - set SP_EL1 for EL1 exception handler
 *   - set SCTLR_EL1
 *   - set VBAR_EL1
 *
 * x0 = current cpu
 * x1 = current el
 * x2 = bottom of stack
 * x3 = size of stack
 *
 *   clobbers x0-x9
 */
_init_cpu_el1:
  mov x9, x30

  /* set SCTLR */
  mov x4, #SCTLR_HI
  lsl x4, x4, #12
  add x4, x4, #SCTLR_LO
  msr sctlr_el1, x4

  /* set SP_EL1 */
  mov x7, #2
  mul x6, x6, x7
  add x2, x2, x6

  cmp x1, #2
  /* if at EL1 then cannot access SP_EL1 directly
   * must use SP
   */
  b.ne 1f
  msr sp_el1, x2
1:
  mov sp, x2
  sub x2, x2, x3
  msr sp_el0, x2

  /* enable FP/ASIMD (for printf/varargs) */
  mov	x4, #(3 << 20)
  msr	cpacr_el1, x4

  /* set VBAR_EL1 */
  adrp x4, el1_exception_vector_table_p0
  add x4, x4, :lo12:el1_exception_vector_table_p0
  mov x5, #4096
  mul x5, x5, x9
  add x4, x4, x5
  msr vbar_el1, x4

  mov x30, x9
  ret

/* perform initialisation of the EL2 state
 *
 * by doing the following:
 *   - set SP_EL2 for EL2 exception handler
 *   - set SCTLR_EL2
 *   - set VBAR_EL2
 *
 * x0 = current cpu
 * x1 = current el (must be #2)
 * x2 = bottom of stack
 * x3 = size of stack
 *
 *   clobbers x0-x9
 */
_init_cpu_el2:
  mov x9, x30

  /* set SCTLR */
  mov x4, #SCTLR_HI
  lsl x4, x4, #12
  add x4, x4, #SCTLR_LO
  msr sctlr_el2, x4

  /* set SP_EL2 */
  mov x7, #3
  mul x3, x3, x7
  add x2, x2, x3
  mov sp, x2

  /* enable FP/ASIMD (for printf/varargs) */
  mov	x4, #(3 << 20)
  msr	cptr_el2, x4

  /* set VBAR_EL2 */
  adrp x4, el2_exception_vector_table_p0
  add x4, x4, :lo12:el2_exception_vector_table_p0
  mov x5, #4096
  mul x5, x5, x9
  add x4, x4, x5
  msr vbar_el2, x4

  mov x30, x9
  ret
