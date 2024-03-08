#define __ASSEMBLY__
#include "asm.h"

.section .init.boot
.globl _start
_start:
  /* get address this was loaded at */
  adrp	x9, _start
  add     x9, x9, :lo12:_start

  /*
   * Since QEMU virt-machine will load the ELF at address 0x40080000  (or wherever...)
   * some of the static-storage-duration objects will be re-located by the loader
   * but their references will not.
   *
   * the .rela.dyn section of the ELF will contain the R_AARCH64_RELATIVE relocations
   * for those references to static-storage-duration objects.
   *
   * there should not be any other relocation types except Elf64_Rela:
   *
   * struct Elf64_Rela {
   * 	uint64_t r_offset;
   * 	uint64_t r_info;
   * 	int64_t  r_addend;
   * }
   *
   * The value at location r_offset stores a reference to the location r_addend
   * relative from the base of the ELF.  The code below just adds the base to
   * all of those references.
   *
   * See https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-54839/index.html
   */
  adrp	x10, __ld_begin_reloc
  add     x10, x10, :lo12:__ld_begin_reloc
  adrp	x11, __ld_end_reloc
  add     x11, x11, :lo12:__ld_end_reloc
1:
  sub x12, x11, x10
  cbz x12, 1f
  ldr x12, [x10]      // r_offset
  ldr x13, [x10, #16] // r_addend
  add x13, x13, x9    // base + r_addend
  str x13, [x9, x12]  // Mem[base + r_offset] <- base + r_addend
  add x10, x10, #24
  b 1b

1:
  /* zero the bss */
  adrp	x10, __ld_begin_bss
  add     x10, x10, :lo12:__ld_begin_bss
  adrp	x11, __ld_end_bss
  add     x11, x11, :lo12:__ld_end_bss
1:
  sub x12, x11, x10
  cbz x12, 1f
  strb wzr, [x10]
  add x10, x10, #1
  b 1b

1:
  /* for spin-table implementations
   * this CPU might not be CPU 0
   * so we divert non-0 CPUs to
   * the spinner
   *
   * this must happen after the previous relocations
   * but it is safe to duplicate that work
   * so we just do this after.
   */
  mrs x9, mpidr_el1
  and x9, x9, #0xff
  cbnz x9, cpu_entry

  /* setup CPU0 */
  mov x29, #0    /* init frame pointer to NULL */
  bl init_cpu
  bl setup
  bl fixup_stack

  /* call main */
  adrp	x0, __argc
  ldr	x0, [x0, :lo12:__argc]
  adrp	x1, __argv
  add	x1, x1, :lo12:__argv
  bl main

  /* exit */
  bl abort
  b halt

.globl cpu_entry
cpu_entry:
  mov x29, #0  /* init frame pointer to NULL */
  bl init_cpu

  mov x0, x9
  bl secondary_init
  bl secondary_idle_loop
  b halt

.globl init_cpu
init_cpu:
 /* each thread has page desginated for stack space
  * where BOT_OF_STACK = ALIGN_UP(__ld_end_sections, 2MiB)
  * located at BOT_OF_STACK + STACK_SIZE*(1+cpuid) -> BOT_OF_STACK + STACK_SIZE*cpuid
  */
  mrs x9, mpidr_el1
  and x9, x9, #0xff
  msr tpidr_el0, x9

  /* x11 = BOT_OF_STACK */
  adrp x11, __ld_end_sections
  add x11, x11, :lo12:__ld_end_sections
  mov x12, #1
  /* x10 = 2MiB = STACK_SIZE */
  mov x10, #1
  lsl x10,x10,#21
  /* align BOT_OF_STACK _up_ to 2M */
  sub x15, x10, #1
  add x11, x11, x15
  add x15,x15,#1
  neg x15,x15
  and x11, x11, x15
  /* x15 = TOP_OF_STACK_EL0 */
  /* x16 = TOP_OF_STACK_EL1 */
  add x15,x9,#1
  lsl x15,x15,#1
  mul x15,x15,x10
  mov x16,x15
  sub x15,x15,x10
  add x15,x15,x11
  add x16,x16,x11

  mov x12, #SCTLR_HI
  lsl x12, x12, #12
  add x12, x12, #SCTLR_LO
  msr sctlr_el1, x12

  /* setup stacks */
  mov sp, x16
  msr sp_el0, x15
  mov x4, #0
  msr spsel, x4  /* and use SP_EL0 outside of exception handlers */

  /* force disable FP/ASIMD */
  msr	cpacr_el1, xzr

  /* set VBAR */
  adrp x14, el1_exception_vector_table_p0
  add x14, x14, :lo12:el1_exception_vector_table_p0
  mov x13, #4096
  mul x13, x13, x9
  add x14, x14, x13
  msr vbar_el1, x14
  isb
  ret

.globl fixup_stack
fixup_stack:
  /* we may have to fix up the `sp` register
    * to point to the MMAP'd region */
  mrs x0, sctlr_el1
  tbz x0, #0, 2f
1:
  mov x0, sp

  adrp x14, BOT_OF_STACK_PA
  add x14, x14, :lo12:BOT_OF_STACK_PA
  ldr x1, [x14]

  adrp x14, TOP_OF_STACK_PA
  add x14, x14, :lo12:TOP_OF_STACK_PA
  ldr x2, [x14]

  /* if SP < phys_bot then definitely some kind of error */
  cmp x0, x1
  b.ls abort

  /* if SP > phys_top then using VM stack, so just return */
  cmp x0, x2
  b.hi 2f

  /* x3 := STACK_MMAP_BASE */
  mov x3, 16
  lsl x3, x3, #30

  sub x0, x0, x1
  add x0, x0, x3
  mov sp, x0
  fallthrough
2:
  ret


.globl halt
halt:
  wfe
  b halt
