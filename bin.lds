/* Adapted from kvm-unit-tests' flat.lds script
 * See http://www.linux-kvm.org/page/KVM-unit-tests for more details
 */
SECTIONS
{
    /* place .text into whole 4k page chunks
     */
    . = ALIGN(4k);
    PROVIDE(__ld_begin_text = .);
    .text : {
        *(.init) *(.text) *(.text.*) *(.exc_table)
    }
    . = ALIGN(4k);
    PROVIDE(__ld_end_text = .);

    /* relocations must be exactly marked
     * so don't align
     */
    .rela.dyn : {
        PROVIDE(__ld_begin_reloc = .);
        *(.rela.dyn)
    }
    PROVIDE(__ld_end_reloc = .);

    . = ALIGN(4k);
    PROVIDE(__ld_start_ro = .);
    .dynsym   : { *(.dynsym) }
    .dynstr   : { *(.dynstr) }
    .hash     : { *(.hash) }
    .gnu.hash : { *(.gnu.hash) }
    .got      : { *(.got) *(.got.plt) }
    .eh_frame : { *(.eh_frame) }

    .rodata   : { *(.rodata*) }
    .data     : { *(.data) }
    .bss      : { *(.bss) }
    . = ALIGN(4k);
    PROVIDE(__ld_end_ro = .);

    . = ALIGN(64K);
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

    /*
     * stack depth is 16K for arm and PAGE_SIZE for arm64, see THREAD_SIZE
     * sp must be 16 byte aligned for arm64, and 8 byte aligned for arm
     * sp must always be strictly less than the true stacktop
     */
    .stack : {
        PROVIDE(__ld_begin_stack = .);
        . += 64K;
        . = ALIGN(64K);
        PROVIDE(stackptr = . - 16);
        PROVIDE(__ld_end_stack = .);
    }

    /DISCARD/ : {
        *(.note*)
        *(.interp)
        *(.comment)
        *(.dynamic)
    }

    PROVIDE(__ld_end_sections = .);
}

ENTRY(_start)