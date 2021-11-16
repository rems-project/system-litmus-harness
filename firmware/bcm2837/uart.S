#include <bcm2837.h>

.section .text.uart

/* rpi3 uart initialisation */
wait:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    mov x0, #150
1:
    sub x0, x0, #1
    cbnz x0, 1b

    ldr x0, [sp, #16]
    ldr x30, [sp], #100
    ret

.globl uart1_init
uart1_init:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str	x9, [sp, #32]

    ldr x9, =GPFSEL1
    ldr w0, [x9]
    // Clear the config for pin 14.
    and w0, w0, ~MASK_U32(12, 3)
    // Clear the config for pin 15.
    and w0, w0, ~MASK_U32(15, 3)
    // Select alternative function 5 for pin 14.
    orr w0, w0, (GPFSEL_ALT5 << 12)
    // Select alternative function 5 for pin 15.
    orr w0, w0, (GPFSEL_ALT5 << 15)
    // write configuration back
    str w0, [x9]

    // We also need to disable pull-up/down for pins 14 and 15.
    ldr x9, =GPPUD
    ldr w0, =GPPUD_OFF
    str w0, [x9]
    // wait 150cycles
    bl wait
    // Assert clock on pins 14 and 15.
    ldr x9, =GPPUDCLK0
    ldr w0, =(BIT_U32(14) | BIT_U32(15))
    str w0, [x9]
    // wait 150cycles
    bl wait
    ldr x9, =GPPUD
    ldr w0, =GPPUD_OFF
    str w0, [x9]
    ldr x9, =GPPUDCLK0
    str wzr, [x9]

    // We can now initialise UART1 (mini UART), which we must first enable.
    ldr x9, =AUX_ENABLES
    ldr w0, [x9]
    orr w0, w0, AUX_ENABLES_BIT_UART1
    str w0, [x9]

    // Configuration of the mini UART.
    ldr x9, =AUX_MU_CNTL_REG
    str wzr, [x9]
    ldr x9, =AUX_MU_LCR_REG
    mov w0, #3
    str w0, [x9]
    ldr x9, =AUX_MU_MCR_REG
    str wzr, [x9]
    ldr x9, =AUX_MU_IER_REG
    str wzr, [x9]
    ldr x9, =AUX_MU_IIR_REG
    mov w0, #6
    str w0, [x9]
    ldr x9, =AUX_MU_BAUD_REG
    mov w0, #270
    str w0, [x9]

    // Finally, enable both Tx and Rx.
    ldr x9, =AUX_MU_CNTL_REG
    mov w0, (AUX_MU_CNTL_RX_ENABLE | AUX_MU_CNTL_TX_ENABLE)
    str w0, [x9]

    ldr x0, [sp, #16]
    ldr	x9, [sp, #32]
    ldr	x30, [sp], #100
    ret

.globl uart1_putc
uart1_putc:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str x1, [sp, #32]
    str x9, [sp, #48]

    /* x0 = byte */
    ldr x9, =AUX_MU_LSR_REG
1:
    ldr w1, [x9]
    and w1, w1, AUX_MU_LSR_TX_EMPTY
    cbz w1, 1b

    ldr x9, =AUX_MU_IO_REG
    str w0, [x9]

    ldr x0, [sp, #16]
    ldr x1, [sp, #32]
    ldr x9, [sp, #48]
    ldr x30, [sp], #100
    ret

/* x0 = u64 */
.globl uart1_puthex
uart1_puthex:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str x1, [sp, #32]
    str x9, [sp, #48]

    mov x1, x0
    ldr x2, =hexdigits
    // cur_nib = 16
    mov x3, #16
    // size = 4
    mov x4, #4
1:
    // cur_nib -= 1
    sub x3, x3, #1
    // shift = cur_nib * size
    mul x5, x3, x4
    // nib = u64 >> shift
    lsr x5, x1, x5
    // nib = nib & 0xf
    and x5, x5, 0xf
    // hex = hexdigits[nib]
    ldrb w0, [x2, x5]
    bl uart1_putc
    cbnz x3, 1b
    
    ldr x0, [sp, #16]
    ldr x1, [sp, #32]
    ldr x9, [sp, #48]
    ldr	x30, [sp], #100
    ret

.globl uart1_puts
uart1_puts:
    /* x0 = char* */
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str x1, [sp, #32]
    str x3, [sp, #48]
#ifdef DEBUG
    mov x3, x0
1:
    ldrb w1, [x3]
    cbz w1, 1f
    mov w0, w1
    bl uart1_putc
    add x3, x3, #1
    b 1b
1:
#endif
    ldr x0, [sp, #16]
    ldr x1, [sp, #32]
    ldr x3, [sp, #48]
    ldr	x30, [sp], #100
    ret

.globl debug_print_welcome_msg
debug_print_welcome_msg:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
#ifdef DEBUG
    ldr x0, =msg_boot
    bl uart1_puts
#endif
    ldr x0, [sp, #16]
    ldr	x30, [sp], #100
    ret

.globl debug_print_el3_setup_done
debug_print_el3_setup_done:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
#ifdef DEBUG
    ldr x0, =msg_el3
    bl uart1_puts
#endif
    ldr x0, [sp, #16]
    ldr	x30, [sp], #100
    ret

.globl debug_print_el2_kernel_run
debug_print_el2_kernel_run:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
#ifdef DEBUG
    ldr x0, =msg_at_el2
    bl uart1_puts
#endif
    ldr x0, [sp, #16]
    ldr	x30, [sp], #100
    ret

.globl debug_print_dtb_addr
debug_print_dtb_addr:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str x1, [sp, #32]
#ifdef DEBUG
    mov x1, x0
    ldr x0, =msg_dtb_addr_lhs
    bl uart1_puts

    mov x0, x1
    bl uart1_puthex

    ldr x0, =msg_dtb_addr_rhs
    bl uart1_puts
#endif
    ldr x0, [sp, #16]
    ldr x1, [sp, #32]
    ldr	x30, [sp], #100
    ret

.globl debug_print_kernel_addr
debug_print_kernel_addr:
    str	x30, [sp, #-100]!
    str x0, [sp, #16]
    str x1, [sp, #32]
    str x7, [sp, #48]
#ifdef DEBUG
    mov x1, x7
    ldr x0, =msg_kernel_addr_lhs
    bl uart1_puts

    mov x0, x1
    bl uart1_puthex

    ldr x0, =msg_kernel_addr_rhs
    bl uart1_puts
#endif
    ldr x0, [sp, #16]
    ldr x1, [sp, #32]
    ldr x7, [sp, #48]
    ldr	x30, [sp], #100
    ret

hexdigits:
    .ascii "01234567890ABCDEF"
   
msg_boot:
/*  .ascii "[                                          ]\n" */
    .ascii "[armstub8 boot                             ]\n"
    .byte 0
msg_el3:
    .ascii "[armstub8 EL3 setup complete               ]\n"
    .byte 0
msg_at_el2:
    .ascii "[armstub8 starting kernel at EL2           ]\n"
    .byte 0
msg_dtb_addr_lhs:
    .ascii "[armstub8 dtb addr:    0x"
    .byte 0
msg_dtb_addr_rhs:
    .ascii "  ]\n"
    .byte 0
msg_kernel_addr_lhs:
    .ascii "[armstub8 kernel addr: 0x"
    .byte 0
msg_kernel_addr_rhs:
    .ascii "  ]\n"
    .byte 0