.section .init

.globl start
start:
	bl setup_stack

	bl main
	bl abort
	b halt

.globl setup_stack
setup_stack:
	mov	x4, #0  /* use SP_EL0 everywhere */
	msr	spsel, x4
	isb
	adrp    x4, stackptr
	add     sp, x4, :lo12:stackptr
	ret

.globl halt
halt:
	b halt