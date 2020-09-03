#define __ASSEMBLY__
#include "asm.h"

.section .init

.global debug_letter_overites_x0_x1

.macro debug_letter_overwrites_x0_x1 , char
        movz    x0, #0x900, lsl #16
        mov     x1, #\char
        str     x1, [x0]
.endm

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
	/* setup CPU0 */
	bl init_cpu
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

	mov x0, x9
	bl secondary_init
	bl secondary_idle_loop
	b halt

.globl init_cpu
init_cpu:
	/* each thread has page desginated for stack space
	 * located at stackptr + 4096*cpuid */
	mrs x9, mpidr_el1
	and x9, x9, #0xff
	msr tpidr_el0, x9

	mov x10, #4096
	mul x10, x9, x10
	adrp x11, stackptr
	add x11, x11, :lo12:stackptr
	add x11, x10, x11

	mov x12, #SCTLR_HI
	lsl x12, x12, #12
	add x12, x12, #SCTLR_LO
	msr sctlr_el1, x12


	/* setup stack */
	mov	x4, #0  /* use SP_EL0 everywhere */
	msr	spsel, x4
	isb
	mov sp, x11

	/* enable FP/ASIMD (for printf/varargs) */
	mov	x4, #(3 << 20)
	msr	cpacr_el1, x4

	/* set VBAR */
	adrp x14, el1_exception_vector_table_p0
	add x14, x14, :lo12:el1_exception_vector_table_p0
	mov x13, #4096
	mul x13, x13, x9
	add x14, x14, x13
    msr vbar_el1, x14
	isb
	ret

.globl halt
halt:
	wfe
	b halt
