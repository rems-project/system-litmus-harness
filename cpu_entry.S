#define __ASSEMBLY__
#include "asm.h"

.section .init.boot
.globl _start
_start:
  /* x0 was pointer to fdt
   * save it off, so we can pass it in later once setup is done
   */
  mov x28, x0

	/* get address this was loaded at */
	adrp	x9, _start
	add x9, x9, :lo12:_start

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

	mrs x9, mpidr_el1
	and x9, x9, #0xff
    cbnz x9, cpu_entry
    */

	/* setup CPU0 */
	bl init_cpu

  mov x29, #0    /* init frame pointer to NULL */
  mov x0, x28    /* arg0 == fdt pointer */
	bl setup

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
	bl init_cpu

	mrs x0, tpidr_el0
  mov x29, #0  /* init frame pointer to NULL */
	bl secondary_init
	bl secondary_idle_loop
	b halt

.globl halt
halt:
	wfe
	b halt
