.section .init

.globl start
start:
	/* get address this was loaded at */
	adrp	x4, start
	add     x4, x4, :lo12:start

	/*
	 * Update all R_AARCH64_RELATIVE relocations using the table
	 * of Elf64_Rela entries between reloc_start/end. The build
	 * will not emit other relocation types.
	 *
	 * struct Elf64_Rela {
	 * 	uint64_t r_offset;
	 * 	uint64_t r_info;
	 * 	int64_t  r_addend;
	 * }
	 *
	 * See https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-54839/index.html
	 */
	adrp	x5, reloc_start
	add     x5, x5, :lo12:reloc_start
	adrp	x6, reloc_end
	add     x6, x6, :lo12:reloc_end
1:
	sub x7, x6, x5
	cbz x7, 1f
	ldr x7, [x5]      // r_offset
	ldr x8, [x5, #16] // r_addend
	add x8, x8, x4    // base + r_addend
	str x8, [x4, x7]  // Mem[base + r_offset] <- base + r_addend
	add x5, x5, #24
	b 1b


1:
	adrp x0, stackptr
	add x0, x0, :lo12:stackptr
	bl setup_stack
	bl setup
	bl init_cpus

	bl main
	bl abort
	b halt

.globl init_cpus
init_cpus:
	str x30, [sp, #8]!
	mov x0, #1
	bl cpu_boot
	mov x0, #2
	bl cpu_boot
	mov x0, #3
	bl cpu_boot
	ldr x30, [sp], #-8
	ret

.globl cpu_entry
cpu_entry:
	/* each thread has page desginated for stack space
	 * located at stackptr + 4096*cpuid */
	mrs x1, mpidr_el1
	and x1, x1, #0xff

	mov x2, #4096
	mul x2, x1, x2
	adrp x3, stackptr
	add x3, x3, :lo12:stackptr
	add x3, x2, x3
	mov x0, x3
	bl setup_stack

	mov x0, x1
	bl secondary_init
	bl secondary_idle_loop
	b halt

.globl setup_stack
setup_stack:
	mov	x4, #0  /* use SP_EL0 everywhere */
	msr	spsel, x4
	isb
	mov sp, x0
	ret

.globl halt
halt:
	wfe
	b halt