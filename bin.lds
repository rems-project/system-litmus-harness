/* Adapted from kvm-unit-tests' flat.lds script
 * See http://www.linux-kvm.org/page/KVM-unit-tests for more details
 */
SECTIONS
{
    .text : { *(.init) *(.text) *(.text.*) }
    . = ALIGN(64K);
    PROVIDE(text_end = .);

    PROVIDE(reloc_start = .);
    .rela.dyn : { *(.rela.dyn) }
    PROVIDE(reloc_end = .);
    .dynsym   : { *(.dynsym) }
    .dynstr   : { *(.dynstr) }
    .hash     : { *(.hash) }
    .gnu.hash : { *(.gnu.hash) }
    .got      : { *(.got) *(.got.plt) }
    .eh_frame : { *(.eh_frame) }

    .rodata   : { *(.rodata*) }
    .data     : { *(.data) }
    .bss      : { *(.bss) }
    . = ALIGN(64K);
    PROVIDE(data_end = .);

    /*
     * stack depth is 16K for arm and PAGE_SIZE for arm64, see THREAD_SIZE
     * sp must be 16 byte aligned for arm64, and 8 byte aligned for arm
     * sp must always be strictly less than the true stacktop
     */
    . = ALIGN(64K);
    PROVIDE(stackptr = . - 16);
    PROVIDE(stacktop = .);

    /DISCARD/ : {
        *(.note*)
        *(.interp)
        *(.debug*)
        *(.comment)
        *(.dynamic)
    }
}

ENTRY(start)