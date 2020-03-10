#include "lib.h"

static void set_block_or_page(uint64_t* root, uint64_t va, uint64_t prot, uint64_t desired_level) {
  vmm_ensure_level(root, desired_level, va);

  uint64_t* block = vmm_block(root, va);
  int level = vmm_level(root, va);
  desc_t final;
  final.type = Block;
  final.oa = va;
  final.level = level;
  final.attrs = read_attrs(prot);
  final.attrs.AF = 1;
  final.attrs.SH = 3;
  *block = write_desc(final);
}


// TODO: maybe this could be when va_start and pa_start aren't aligned.
void ptable_set_idrange(uint64_t* root, 
                        uint64_t va_start, uint64_t va_end,
                        uint64_t prot) {
  uint64_t level1 = 30, level2 = 21, level3 = 12;

  if (!IS_ALIGNED(va_start, level3)) {
    puts("! error: ptable_set_idrange: got unaligned va_start\n");
    abort();
  }

  if (!IS_ALIGNED(va_end, level3)) {
    puts("! error: ptable_set_idrange: got unaligned va_end\n");
    abort();
  }

  lock(&vmm_lock);

  uint64_t va = va_start; /* see above: must be aligned on a page */

  for (; !IS_ALIGNED(va, level2) && va < va_end;
       va += (1UL << level3))
    set_block_or_page(
        root, va, prot,
        3);  // allocate 4k regions up to the first 2M region

  for (; !IS_ALIGNED(va, level1) && va < va_end;
       va += (1UL << level2))
    set_block_or_page(
        root, va, prot,
        2);  // allocate 2M regions up to the first 1G region

  for (; va < ALIGN_TO(va_end, level1);
       va += (1UL << level1))
    set_block_or_page(root, va, prot,
                                1);  // Alloc as many 1G regions as possible

  for (; va < ALIGN_TO(va_end, level2);
       va += (1UL << level2))
    set_block_or_page(
        root, va, prot,
        2);  // allocate as much of what's left as 2MB regions

  for (; va < va_end; va += (1UL << level3))
    set_block_or_page(root, va, prot,
                                3);  // allocate whatever remains as 4k pages.

  unlock(&vmm_lock);
}


uint64_t* vmm_alloc_new_idmap_4k(void) {
  uint64_t* root_ptable = alloc(4096);
  valloc_memset(root_ptable, 0, 4096);

  /* set ranges according to kvm-unit-tests/lib/arm/mmu.c */
  uint64_t phys_offs = (1UL << 30); /* first 1 GiB region */
  uint64_t phys_end = (3UL << 30);  /* up to 3GiB */

  /* QEMU Memory Mapped I/O
   * 0x00000000 -> 0x08000000  == Boot ROM
   * 0x08000000 -> 0x09000000  == GIC
   * 0x09000000 -> 0x09001000  == UART
   * ....       -> 0x40000000  == MMIO/PCIE/etc
   *
   * see https://github.com/qemu/qemu/blob/master/hw/arm/virt.c
   *
   * Actual RAM is from
   * 0x40000000 -> RAM_END
   *  where RAM_END is defined by the dtb (but is < 3GiB)
   */
  ptable_set_idrange(root_ptable, 0, phys_offs, 0x44);

  /* .text segment (code) */
  /* AArch64 requires that code executable at EL1 is not writable at EL0 */
  ptable_set_idrange(root_ptable, phys_offs, TOP_OF_TEXT, 0x00);

  /* bss and other data segments */
  ptable_set_idrange(root_ptable, TOP_OF_TEXT, TOP_OF_DATA, 0x50);

  /* stack */
  ptable_set_idrange(root_ptable, BOT_OF_STACK, TOP_OF_STACK, 0x50);

  /* heap */
  ptable_set_idrange(root_ptable, BOT_OF_HEAP, TOP_OF_MEM, 0x50);

  return root_ptable;
}

static void set_new_ttable(uint64_t ttbr, uint64_t tcr) {
  vmm_mmu_off();
  write_sysreg(ttbr, ttbr0_el1);
  write_sysreg(tcr, tcr_el1);
  dsb();
  isb();
  vmm_mmu_on();
}

void vmm_set_id_translation(uint64_t* pgtable) {
  /* now set the new TTBR and TCR */
  uint64_t ttbr = (uint64_t)pgtable;
  uint64_t tcr = (0L << 39) | /* HA, software access flag */
                 (1L << 37) | /* TBI, top byte ignored. */
                 (5L << 32) | /* IPS, 48-bit (I)PA. */
                 (0 << 14) |  /* TG0, granule size, 4K. */
                 (3 << 12) |  /* SH0, inner shareable. */
                 (1 << 10) |  /* ORGN0, normal mem, WB RA WA Cacheable. */
                 (1 << 8) |   /* IRGN0, normal mem, WB RA WA Cacheable. */
                 (16 << 0) |  /* T0SZ, input address is 48 bits => VA[47:12] are
                                 used for  translation starting at level 0. */
                 0;

  set_new_ttable(ttbr, tcr);
}

void __vmm_free_pgtable(uint64_t* pgtable, int level) {
  for (int i = 0; i < 512; i++) {
    desc_t d = read_desc(*(pgtable + i), level);
    if (d.type == Table) {
      __vmm_free_pgtable((uint64_t*)d.table_addr, level+1);
    }
  }

  free(pgtable);
}

void vmm_free_pgtable(uint64_t* pgtable) {
  __vmm_free_pgtable(pgtable, 0);
}