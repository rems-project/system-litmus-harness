.section .exc_table

.macro save_all_registers
    stp x0,     x1,     [sp, #-288]!
    stp x2,     x3,     [sp, #16]
    stp x4,     x5,     [sp, #32]
    stp x6,     x7,     [sp, #48]
    stp x8,     x9,     [sp, #64]
    stp x10,    x11,    [sp, #80]
    stp x12,    x13,    [sp, #96]
    stp x14,    x15,    [sp, #112]
    stp x16,    x17,    [sp, #128]
    stp x18,    x19,    [sp, #144]
    stp x20,    x21,    [sp, #160]
    stp x22,    x23,    [sp, #176]
    stp x24,    x25,    [sp, #192]
    stp x26,    x27,    [sp, #208]
    stp x28,    x29,    [sp, #224]
    str x30,            [sp, #240]
    mov x0, sp
    add x0, x0, #288
    str x0,             [sp, #248]
.endm

.macro restore_all_registers
    ldp x2,     x3,     [sp, #16]
    ldp x4,     x5,     [sp, #32]
    ldp x6,     x7,     [sp, #48]
    ldp x8,     x9,     [sp, #64]
    ldp x10,    x11,    [sp, #80]
    ldp x12,    x13,    [sp, #96]
    ldp x14,    x15,    [sp, #112]
    ldp x16,    x17,    [sp, #128]
    ldp x18,    x19,    [sp, #144]
    ldp x20,    x21,    [sp, #160]
    ldp x22,    x23,    [sp, #176]
    ldp x24,    x25,    [sp, #192]
    ldp x26,    x27,    [sp, #208]
    ldp x28,    x29,    [sp, #224]
    ldr x30,            [sp, #240]
    ldp x0,     x1,     [sp], #288
.endm

.macro handler, name, vec
\name:
    save_all_registers
    mov x0, #\vec
    mrs x1, esr_el1
    mov x2, sp

    /* make fake stack frame
     * which directs to the faulty instruction
     */
    mrs x3, elr_el1
    stp x2, x3, [sp, #-0x10]!
    bl handle_exception
    add sp, sp, #0x10

    restore_all_registers
    eret
.endm

.macro restore_other_registers
    ldp x8,     x9,     [sp, #64]
    ldp x10,    x11,    [sp, #80]
    ldp x12,    x13,    [sp, #96]
    ldp x14,    x15,    [sp, #112]
    ldp x16,    x17,    [sp, #128]
    ldp x18,    x19,    [sp, #144]
    ldp x20,    x21,    [sp, #160]
    ldp x22,    x23,    [sp, #176]
    ldp x24,    x25,    [sp, #192]
    ldp x26,    x27,    [sp, #208]
    ldp x28,    x29,    [sp, #224]
    ldr x30,            [sp, #240]
    add sp, sp, #288
.endm

handler el1_sp0_sync,           0
handler el1_sp0_irq,            1
handler el1_sp0_fiq,            2
handler el1_sp0_serror,         3

handler el1_spx_sync,           4
handler el1_spx_irq,            5
handler el1_spx_fiq,            6
handler el1_spx_serror,         7

handler el0_64_sync,            8
handler el0_64_irq,             9
handler el0_64_fiq,            10
handler el0_64_serror,         11

handler el0_32_sync,           12
handler el0_32_irq,            13
handler el0_32_fiq,            14
handler el0_32_serror,         15

.macro vectorjmp, name
.align 7
b \name
.endm

.macro vector_table, name
.global \name
.align 12  /* align to a page so that we can do pagetable trickery later */
\name:
    vectorjmp el1_sp0_sync
    vectorjmp el1_sp0_irq
    vectorjmp el1_sp0_fiq
    vectorjmp el1_sp0_serror

    vectorjmp el1_spx_sync
    vectorjmp el1_spx_irq
    vectorjmp el1_spx_fiq
    vectorjmp el1_spx_serror

    vectorjmp el0_64_sync
    vectorjmp el0_64_irq
    vectorjmp el0_64_fiq
    vectorjmp el0_64_serror

    vectorjmp el0_32_sync
    vectorjmp el0_32_irq
    vectorjmp el0_32_fiq
    vectorjmp el0_32_serror
.endm

/* Each PE has its own table */
vector_table el1_exception_vector_table_p0
vector_table el1_exception_vector_table_p1
vector_table el1_exception_vector_table_p2
vector_table el1_exception_vector_table_p3