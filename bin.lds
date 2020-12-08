/* Adapted from kvm-unit-tests' flat.lds script
 * See http://www.linux-kvm.org/page/KVM-unit-tests for more details
 */
SECTIONS
{
    /* on the rpi4 and in QEMU's mach-virt implementation
     * RAM starts from physical address 0x40000000
     *
     * then the bootloader will load _start at address 0x40080000
     *
     * so if we wish to place a section @ 0x40ABCDEF
     * then we have to place it at location 0x0ABCDEF - 0x80000
     * in the linker script
     */
    OFFSET = 0x80000;

    /* place all the shared data in the next 1G region
     * this is data that will be mapped and
     * note that 0x4000000 is the bootloader itself which does not actually need to be mapped
     * (since it is used before we install the pgtable) so we can consider 0x40000000 -> ... as one big
     * code segment
     */
    .text 0x00000000 : {
        PROVIDE(__ld_begin_text = .);
        *(.init) *(.text) *(.text.*) *(.exc_table)

        . = ALIGN(4k);
        PROVIDE(__ld_end_text = .);
    }

    /* relocations must be exactly marked
     * so don't align any more than needed
     */
    . = ALIGN(4k);
    .rela.dyn : {
        PROVIDE(__ld_begin_reloc = .);
        *(.rela.dyn)
        /* PROVIDE(__ld_end_reloc = .); */
    }
    /* TODO: this seems to need to go after the .rela.dyn section
     * but it should be fine to put it after the *(.rela.dyn) above
     * but it doesn't work ?
     */
    PROVIDE(__ld_end_reloc = .);

    . = ALIGN(4k);
    PROVIDE(__ld_begin_data = .);
    .dynsym   : { *(.dynsym) }
    .dynstr   : { *(.dynstr) }
    .hash     : { *(.hash) }
    .gnu.hash : { *(.gnu.hash) }
    .got      : { *(.got) *(.got.plt) }
    .eh_frame : { *(.eh_frame) }

    .rodata   : { *(.rodata*) }
    .data     : { *(.data) }
    .bss      : {
        PROVIDE(__ld_begin_bss = .);
        *(.bss)
        PROVIDE(__ld_end_bss = .);
    }
    . = ALIGN(4k);
    PROVIDE(__ld_end_data = .);

    /*
     * we ensure at least 2 2M pmd for each thread's stack
     * (one for EL0 and one for EL1)
     *
     * see device.h
     */
    . = ALIGN(4K);
    .stack : {
        PROVIDE(__ld_begin_stack = .);
        . += 2M;  /* alignment space */
        . += 16M;
        PROVIDE(__ld_end_stack = .);
    }

    /DISCARD/ : {
        *(.note*)
        *(.interp)
        *(.comment)
        *(.dynamic)
    }

    PROVIDE(__ld_end_sections = .);

    /* from default linker script: `aarch64-none-elf-ld --verbose`
     * DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we place them at 0
     */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end ) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* DWARF 3 */
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges   0 : { *(.debug_ranges) }
    /* DWARF Extension.  */
    .debug_macro    0 : { *(.debug_macro) }
    .debug_addr     0 : { *(.debug_addr) }
    /* end debug sections */
}

ENTRY(_start)