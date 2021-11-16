#include <arm64.h>

/* we define a small amount of stack space */
#define STACK_ADDR (0x1000)

.globl _start
_start:
    nop
    /* this code runs on all cores right at boot
     * at EL3
     *
     * do any initial register setup here
     * (I've probably missed a lot ...)
     */
    
    /* save which CPU we are */
    mrs x10, MPIDR_EL1
    and x10, x10, #3
    cbnz x10, 1f

    /* create some stack space just for us */
    msr spsel, xzr
    ldr x0, =STACK_ADDR
    mov sp, x0
    bl uart1_init
    bl debug_print_welcome_msg
1:
    /* Enable SIMD
     */
    mov x0, xzr
    msr CPTR_EL3, x0

    /* Set up SCR */
    ldr x0, =SCR
    msr SCR_EL3, x0

    /* the BCM2837 SoC uses the Arm A53 core
     * the A53's TRM says we *must* set CPUECTLR_EL1.SMPEN to 1 before enabling caches
     * and to do that we need to enable access to the CPUECTLR_EL1 register in the impl. defined.
     * ACTLR_EL3 register (bit #) */
    ldr x0, =ACTLR
    ldr x1, =CPUECTLR
    msr ACTLR_EL3, x0
    msr CPUECTLR_EL1, x0

    /* Drop to EL2 */
    ldr x0, =SCTLR
    ldr x1, =SPSR
    adr x2, 1f
    msr SCTLR_EL2, x0
    msr SPSR_EL3, x1
    msr ELR_EL3, x2

    cbnz x10, 0f
    bl debug_print_el3_setup_done
0:
    eret
1:  /* now @ EL2 */
    /* if CPU#0,
     * run kernel8.img
     */
    cbz x10, run_kernel

    /* else, spin */
1:
    adr x1, spintable
    wfe
    /* if (u64*)spintable[cpu] == NULL
     *  loop */
    ldr x7, [x1, x10, lsl #3]
    cbz x7, 1b
    /* else, run the code it points to */
    mov x0, #0
    b run_main

/* make sure these locations match the ones in the rpi3 dts
 * so that when you read the dtb you see the right locations ...
 */


.org 0xf0
// the loader scans for the 0x5afe570b word (le) at 0xF0
// let u64 *P = 0xF0
// then the loader goes places the dtb/addr addresses at u64offs +1 and +2 respectively
magic:
    .word 0x5afe570b    // magic number
    .word 0             // version
    .quad 0             // dtb addr output
    .quad 0             // kernel addr output

.org 0x150
spintable:
spin_cpu0:
    .quad 0

.org 0x158
spin_cpu1:
    .quad 0

.org 0x160
spin_cpu2:
    .quad 0

.org 0x168
spin_cpu3:
    .quad 0

.org 0x200

run_kernel:
    bl debug_print_el2_kernel_run
    adr x9, magic

    // load dtb addr
    ldr w0, [x9, #8]
    bl debug_print_dtb_addr

    // load kernel addr
    ldr w7, [x9, #12]
    bl debug_print_kernel_addr

run_main:
    mov x1, #0
    mov x2, #0
    mov x3, #0
    mov x4, #0
    mov x5, #0
    mov x6, #0
    br x7
