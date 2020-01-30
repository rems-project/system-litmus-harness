#define __ASSEMBLY__

.section .init

.globl start
start:
	bl main
	b halt

.globl halt
halt:
	b halt