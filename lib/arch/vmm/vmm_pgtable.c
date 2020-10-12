#include "lib.h"

uint64_t vmm_make_desc(uint64_t pa, uint64_t prot, int level) {
  desc_t final;
  final.type = Block;
  final.oa = pa;
  final.level = level;
  final.attrs = read_attrs(prot);
  final.attrs.nG = PROT_NG_NOT_GLOBAL;
  final.attrs.AF = PROT_AF_ACCESS_FLAG_DISABLE;
  final.attrs.SH = PROT_SH_ISH;
  final.attrs.NS = PROT_NS_NON_SECURE;
  return write_desc(final);
}

static void set_block_or_page(uint64_t* root, uint64_t va, uint64_t pa, uint8_t unmap, uint64_t prot, uint64_t desired_level) {
  vmm_ensure_level(root, desired_level, va);

  desc_t desc = vmm_translation_walk(root, va);

  if (! unmap)
    *desc.src = vmm_make_desc(pa, prot, desc.level);
  else
    *desc.src = 0;
}

void vmm_update_mapping(uint64_t* pgtable, uint64_t va, uint64_t pa, uint64_t prot) {
  debug("for VA=%p => PA=%p, prot=%p\n", va, pa, prot);
  set_block_or_page(pgtable, va, pa, 0, prot, 3);
}

void vmm_unmap_page(uint64_t* pgtable, uint64_t va) {
  debug("unmap VA=%p\n", va);
  set_block_or_page(pgtable, va, 0, 1, 0, 0);
}

void __ptable_set_range(uint64_t* root,
                      uint64_t pa_start,
                      uint64_t va_start, uint64_t va_end,
                      uint8_t unmap,
                      uint64_t prot) {
  uint64_t level1 = 30, level2 = 21, level3 = 12;

  if (unmap) {
    debug("unmap from %p -> %p\n", va_start, va_end);
  } else {
    debug("map %p -> %p, prot=%p\n", va_start, va_end, prot);
  }


  if (!IS_ALIGNED(va_start, level3)) {
    puts("! error: __ptable_set_range: got unaligned va_start\n");
    abort();
  }

  if (!IS_ALIGNED(va_end, level3)) {
    puts("! error: __ptable_set_range: got unaligned va_end\n");
    abort();
  }

  uint64_t va = va_start; /* see above: must be aligned on a page */
  uint64_t pa = pa_start;

  for (; !IS_ALIGNED(va, level2) && va+(1UL << level3) < va_end;
       va += (1UL << level3), pa += (1UL << level3))
    set_block_or_page(
        root, va, pa, unmap, prot,
        3);  // allocate 4k regions up to the first 2M region


  debug("allocated lvl3 up to %p\n", va);

  for (; !IS_ALIGNED(va, level1) && va+(1UL<<level2) < va_end;
       va += (1UL << level2), pa += (1UL << level2))
    set_block_or_page(
        root, va, pa, unmap, prot,
        2);  // allocate 2M regions up to the first 1G region

  debug("allocated lvl2 up to %p\n", va);

  for (; va < ALIGN_TO(va_end, level1) && va+(1UL << level1) < va_end;
       va += (1UL << level1), pa += (1UL << level1))
    set_block_or_page(root, va, pa, unmap, prot,
                                1);  // Alloc as many 1G regions as possible

  debug("allocated lvl1 up to %p\n", va);

  for (; va < ALIGN_TO(va_end, level2) && va+(1UL << level2) < va_end;
       va += (1UL << level2), pa += (1UL << level2))
    set_block_or_page(
        root, va, pa, unmap, prot,
        2);  // allocate as much of what's left as 2MB regions

  debug("allocated lvl2 up to %p\n", va);

  for (; va < va_end; va += (1UL << level3), pa += (1UL << level3))
    set_block_or_page(root, va, pa, unmap, prot,
                                3);  // allocate whatever remains as 4k pages.

  debug("allocated lvl3 up to %p\n", va);
}

void ptable_map_range(uint64_t* root,
                      uint64_t pa_start,
                      uint64_t va_start, uint64_t va_end,
                      uint64_t prot) {
    __ptable_set_range(root, pa_start, va_start, va_end, 0, prot);
}

void ptable_unmap_range(uint64_t* root,
                      uint64_t pa_start,
                      uint64_t va_start, uint64_t va_end) {
    __ptable_set_range(root, pa_start, va_start, va_end, 1, 0);
}


uint64_t* vmm_alloc_new_4k_pgtable(void) {
  uint64_t* root_ptable = alloc(4096);
  valloc_memset(root_ptable, 0, 4096);

  uint64_t phys_offs = (1UL << 30); /* first 1 GiB region */

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
   *  where RAM_END is defined by the dtb
   */
  ptable_map_range(root_ptable, 0, 0, phys_offs, PROT_MEMTYPE_DEVICE | PROT_RW_RWX);

  /* fixed linker sections are identically mapped
   */

  /* .text segment (code) */
  /* AArch64 requires that code executable at EL1 is not writable at EL0 */
  ptable_map_range(root_ptable, phys_offs, phys_offs, TOP_OF_TEXT, PROT_MEMTYPE_NORMAL | PROT_RX_RX);

  /* bss and other data segments */
  ptable_map_range(root_ptable, BOT_OF_RDONLY, BOT_OF_RDONLY, TOP_OF_RDONLY, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);

  /* stack */
  ptable_map_range(root_ptable, BOT_OF_STACK, BOT_OF_STACK, TOP_OF_STACK, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);

  /* heap, for non-test data */
  ptable_map_range(root_ptable, BOT_OF_HEAP, BOT_OF_HEAP, TOP_OF_HEAP, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);

  /* testdata physical address space itself is unmapped in the virtual space */
  ptable_unmap_range(root_ptable, BOT_OF_TESTDATA, BOT_OF_TESTDATA, TOP_OF_TESTDATA);

  /* the harness itself maps all of memory starting @ 64G
   */
  ptable_map_range(root_ptable, phys_offs, (uint64_t)HARNESS_MMAP, (uint64_t)HARNESS_MMAP+TOTAL_MEM, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);

  /* finally, the test data (variables etc) are placed into
   * physical address space TOP_OF_HEAP -> TOP_OF_MEM
   *
   * we allocate 16 x 8M contiguous regions
   * but spread evenly throughout the 128G VA space
   */
  for (int i = 0 ; i < NR_REGIONS; i++) {
    uint64_t start_va = TESTDATA_MMAP_8M_VA_FROM_INDEX(i);
    uint64_t start_pa = TESTDATA_MMAP_8M_PA_FROM_INDEX(i);
    ptable_map_range(root_ptable, start_pa, start_va, start_va + REGION_SIZE, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);
  }

  return root_ptable;
}

static void set_new_ttable(uint64_t ttbr, uint64_t tcr, uint64_t mair) {
  /* no printf happens here, so need to worry about disabling locking during them */
  if ((read_sysreg(sctlr_el1) & 1) == 1) {
    printf("! err: set_new_ttable:  MMU already on!\n");
    abort();
  }
  write_sysreg(ttbr, ttbr0_el1);
  write_sysreg(tcr, tcr_el1);
  write_sysreg(mair, mair_el1);
  dsb();
  isb();
  vmm_mmu_on();
}

void vmm_set_id_translation(uint64_t* pgtable) {
  if (pgtable == NULL) {
    vmm_mmu_off();
    return;
  }

  /* now set the new TTBR and TCR */
  uint64_t asid = 0;
  uint64_t ttbr = TTBR0(pgtable, asid);

  uint64_t tcr = \
    0          |
    (0L << 39) |  /* HA, software access flag */
    (1L << 37) |  /* TBI, top byte ignored. */
    (5L << 32) |  /* IPS, 48-bit (I)PA. */
    (0 << 14)  |  /* TG0, granule size, 4K. */
    (3 << 12)  |  /* SH0, inner shareable. */
    (1 << 10)  |  /* ORGN0, normal mem, WB RA WA Cacheable. */
    (1 << 8)   |  /* IRGN0, normal mem, WB RA WA Cacheable. */
    (16 << 0)  ;  /* T0SZ, input address is 48 bits => VA[47:12] are
                      used for  translation starting at level 0. */


  #define MAIR_ATTR(idx, attr)  ((attr) << (idx*8))
  uint64_t mair = \
    MAIR_ATTR(PROT_ATTR_DEVICE_nGnRnE, MAIR_DEVICE_nGnRnE)  |
    MAIR_ATTR(PROT_ATTR_DEVICE_GRE, MAIR_DEVICE_GRE)        |
    MAIR_ATTR(PROT_ATTR_NORMAL_NC, MAIR_NORMAL_NC)          |
    MAIR_ATTR(PROT_ATTR_NORMAL_RA_WA, MAIR_NORMAL_RA_WA)    |
    0;

  set_new_ttable(ttbr, tcr, mair);
}

void vmm_switch_ttable(uint64_t* new_table) {
  write_sysreg(TTBR0(new_table, 0), ttbr0_el1);
  dsb();
  isb();
  vmm_flush_tlb();
}

void vmm_switch_asid(uint64_t asid) {
  uint64_t ttbr = read_sysreg(ttbr0_el1);
  write_sysreg(TTBR0(ttbr, asid), ttbr0_el1);
  isb();  /* is this needed? */
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