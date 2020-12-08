/** some simple macros that can be used to debug asm code in
 * QEMU
 *
 */

.macro debug_letter_overwrites_x0_x1 , char
        movz    x0, #0x900, lsl #16
        mov     x1, #\char
        str     x1, [x0]
.endm

.global asmprintchar
asmprintchar:
        adrp x9, asmprintchar_table
        add  x9, x9, :lo12:asmprintchar_table

        mov  x10, #16
        mul  x0, x10, x0
        add  x9, x9, x0
        br   x9

asmprintchar_table:
        debug_letter_overwrites_x0_x1 48 // 0
	ret
        debug_letter_overwrites_x0_x1 49 // 1
	ret
        debug_letter_overwrites_x0_x1 50 // 2
	ret
        debug_letter_overwrites_x0_x1 51 // 3
	ret
        debug_letter_overwrites_x0_x1 52 // 4
	ret
        debug_letter_overwrites_x0_x1 53 // 5
	ret
        debug_letter_overwrites_x0_x1 54 // 6
	ret
        debug_letter_overwrites_x0_x1 55 // 7
	ret
        debug_letter_overwrites_x0_x1 56 // 8
	ret
        debug_letter_overwrites_x0_x1 57 // 9
	ret
        debug_letter_overwrites_x0_x1 65 // A
	ret
        debug_letter_overwrites_x0_x1 66 // B
	ret
        debug_letter_overwrites_x0_x1 67 // C
	ret
        debug_letter_overwrites_x0_x1 68 // D
	ret
        debug_letter_overwrites_x0_x1 69 // E
	ret
        debug_letter_overwrites_x0_x1 70 // F
	ret

asmphex_step:
    mov x29, x30
    mov x0, x2
    lsr x0, x0, #60
    // x0 is 4 msb of x2
    lsl x2, x2, #4
    bl 	asmprintchar
    mov x30,x29
    ret

asmputhex:
    mov x28, x30

    mov x2, x0

    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step
    bl asmphex_step

    mov x30, x28
    ret