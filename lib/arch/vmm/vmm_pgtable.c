#include "lib.h"

/** here we define the translation table and virtual memory space
 *
 *
 * the address space on e.g. the rpi4 with the given
 * linker script (and 1 GiB of RAM) is given below
 *
 * addresses are of the form
 *  0xVA AT(0xPA)
 * if no PA is specified, the VA = PA
 *
 * the virtual address space is partitioned like so:
 *  the lower 1G region is IO-MMU mapped region
 *  the  next 1G is id-mapped code,data,bss,relocations,heap and physical stack space
 *  the  next 1G+ is id-mapped testdata
 *  the  next 1G is thread-specific mappings
 *  at 64G is a 64G id-map of all of physical memory
 *  at 128G is a 64G mapping of test data (only when running tests)
 *
 * since the 4k pages means levels of the pagetable maps regions of size:
 *  L0 - 512G
 *  L1 - 1G
 *  L2 - 2M
 *  L3 - 4K
 *
 * this mapping ensures *all* of the harness fits in 1 L0 entry,
 * the first L1 mapping can be shared between all tables
 * and the following L1 mappings can be allocated on a per-thread or per-test-run basis.
 *
 * remaining L0 entries can be used for IO-mapped highmem regions if needed later on,
 * but are not used by the harness.
 *
 *
 *  TOP OF MEM
 *
 *  (then we fill the next NO_CPUS*PAGE_SIZE bytes with the stack
 *   which is loaded here,  but whose virtual address is at the 2G+ region)
 *
 *  (we fill the rest of memory (up to the beginning of the stack space)
 *   with the test data section)
 *  -------------- 0x8000_0000  (RAM_END aka 2 * GiB)
 * |
 * |
 * |  TEST DATA
 * |
 * |
 *  --------------- 0x4880_0000
 *
 *  --------------- 0x48a00000 AT(0x8000_0000  (RAM_END))
 * |
 * |  STACK
 * |
 *  --------------- 0x4880_0000 AT(0x7fe0_0000)
 *
 *  (then fill the next 128M with a shared heap, that
 *   threads can use for shared data allocation)
 *  --------------- 0x4880_0000  (1 GiB + 8 MiB + 128 MiB)
 * |
 * |  HEAP+PTABLES
 * |
 *  --------------- 0x4060_0000
 *
 *  (we stick the data in a 2M region)
 *  --------------- 0x4060_0000
 * |
 * | DATA & BSS
 * |
 *  ~~~~~~~~~~~~~~~ 0x4040_0000  (1 GiB + 4 MiB)
 *
 *  (then a 2M region with relocations (expected mostly zero))
 *  ~~~~~~~~~~~~~~~
 * | Relocations
 *  ~~~~~~~~~~~~~~~
 *
 *  (we fit all of the code in the first 2M region above the IO region)
 *  ~~~~~~~~~~~~~~~ 0x4020_0000
 * |
 * | Text & Vector Table
 * |
 *  --------------- 0x4000_0000  (1 GiB)
 * |
 * | IO Region
 * |
 * ---------------- BOT OF MEM 0x0000_0000
 */

#define TRACE_PTABLE(...) DEBUG(DEBUG_PTABLE, __VA_ARGS__)

u64 vmm_make_desc(u64 pa, u64 prot, int level) {
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

static void set_block_or_page(u64* root, u64 va, u64 pa, u8 unmap, u64 prot, u64 desired_level) {
  vmm_ensure_level(root, desired_level, va);

  desc_t desc = vmm_translation_walk(root, va);

  if (!unmap)
    *desc.src = vmm_make_desc(pa, prot, desc.level);
  else
    *desc.src = 0;
}

static void __ptable_set_range(u64* root, u64 pa_start, u64 va_start, u64 va_end, u8 unmap, u64 prot) {
  u64 level1 = 30, level2 = 21, level3 = 12;

  u64 c = 0;

  if (unmap) {
    TRACE_PTABLE("unmap from %p -> %p\n", va_start, va_end);
  } else {
    TRACE_PTABLE("map %p -> %p, prot=%p\n", va_start, va_end, prot);
  }

  if (!IS_ALIGNED(va_start, level3)) {
    fail("! error: __ptable_set_range: got unaligned va_start\n");
  }

  if (!IS_ALIGNED(va_end, level3)) {
    fail("! error: __ptable_set_range: got unaligned va_end\n");
  }

  u64 va = va_start;
  u64 pa = pa_start;

#define BOTH_ALIGNED(va, pa, level) (IS_ALIGNED((va), (level)) && IS_ALIGNED((pa), (level)))

#define END_OF_REGION(va, level) (((va) + (1UL << (level))) > va_end)

#define INCREMENT(va, pa, level) va += (1UL << (level)), pa += (1UL << (level))

  /* allocate level3 entry until *both* va and pa
   * are aligned on a level2, or until we've allocated everything
   */
  for (c = 0; !BOTH_ALIGNED(va, pa, level2) && !END_OF_REGION(va, level3); INCREMENT(va, pa, level3), c++)
    set_block_or_page(root, va, pa, unmap, prot,
                      3); // allocate 4k regions up to the first 2M region

  TRACE_PTABLE("allocated %ld lvl3 entries up to %p\n", c, va);

  /* Note:
   * if PA has smaller alignment than VA
   * for the level it's being inserted
   * then things will fail
   *
   * e.g. for a level2 (2M) entry you cannot supply a 4k-aligned address
   * it must be 2M aligned otherwise the lower bits will be cut off
   * during the _hardware_ translation walk
   */
  if (IS_ALIGNED(va, level2) && !IS_ALIGNED(pa, level2) && !END_OF_REGION(va, level3)) {
    fail("translation entry misalignment, va=%p, pa=%p. See above comment.\n", va, pa);
  }

  for (c = 0; !BOTH_ALIGNED(va, pa, level1) && !END_OF_REGION(va, level2); INCREMENT(va, pa, level2), c++)
    set_block_or_page(root, va, pa, unmap, prot,
                      2); // allocate 2M regions up to the first 1G region

  TRACE_PTABLE("allocated %ld lvl2 entries up to %p\n", c, va);

  for (c = 0; va < ALIGN_TO(va_end, level1) && !END_OF_REGION(va, level1); INCREMENT(va, pa, level1), c++)
    set_block_or_page(root, va, pa, unmap, prot,
                      1); // Alloc as many 1G regions as possible

  TRACE_PTABLE("allocated %ld lvl1 entries up to %p\n", c, va);

  for (c = 0; va < ALIGN_TO(va_end, level2) && !END_OF_REGION(va, level2); INCREMENT(va, pa, level2), c++)
    set_block_or_page(root, va, pa, unmap, prot,
                      2); // allocate as much of what's left as 2MB regions

  TRACE_PTABLE("allocated %ld lvl2 entries up to %p\n", c, va);

  for (c = 0; va < va_end; INCREMENT(va, pa, level3), c++)
    set_block_or_page(root, va, pa, unmap, prot,
                      3); // allocate whatever remains as 4k pages.

  TRACE_PTABLE("allocated %ld lvl3 entries up to %p\n", c, va);
}

static void ptable_map_range(u64* root, u64 pa_start, u64 va_start, u64 va_end, u64 prot) {
  TRACE_PTABLE("mapping from %p -> %p with translation to %p\n", va_start, va_end, pa_start);
  __ptable_set_range(root, pa_start, va_start, va_end, 0, prot);
}

void vmm_ptable_map(u64* pgtable, VMRegion reg) {
  u64 size = reg.va_end - reg.va_start;
  u64 pa_start = reg.pa_start != 0 ? reg.pa_start : reg.va_start;
  debug("map 0x%lx bytes for region %p -> %p with translation at %p\n", size, reg.va_start, reg.va_end, pa_start);
  ptable_map_range(pgtable, pa_start, reg.va_start, reg.va_end, reg.memattr | reg.prot);
}

void vmm_ptable_unmap(u64* pgtable, VMRegion reg) {
  u64 level;
  u64 size = reg.va_end - reg.va_start;
  debug("unmap 0x%lx bytes at %p\n", size, reg.va_start);
  switch (size) {
  case PAGE_SIZE:
    level = 3;
    break;
  case PMD_SIZE:
    level = 2;
    break;
  case PUD_SIZE:
    level = 1;
    break;
  default:
    fail(
      "! vmm_ptable_unmap can only unmap PAGE/PMD/PUD size chunk (0x%lx, 0x%lx, 0x%lx respectively)\n",
      PAGE_SIZE,
      PMD_SIZE,
      PUD_SIZE
    );
  }
  set_block_or_page(pgtable, reg.va_start, 0, 1, 0, level);
}

const char* VMRegionTag_names[] = {
  "VM_MMAP_IO",  "VM_TEXT",         "VM_DATA",           "VM_STACK",          "VM_HEAP",        "VM_PTABLES",
  "VM_TESTDATA", "VM_MMAP_HARNESS", "VM_MMAP_STACK_EL0", "VM_MMAP_STACK_EL1", "VM_MMAP_VTABLE",
};

static void update_table_from_vmregion_map(u64* table, VMRegions regs) {
  VMRegion* map = regs.regions;

  VMRegion r_prev = { 0 };
  for (int i = VM_MMAP_IO; i <= VM_MMAP_VTABLE; i++) {
    VMRegion r = map[i];

    if (!r.valid)
      continue;

    /* sanity check for overlaps */
    if (r.va_start < r_prev.va_end)
      fail("! %o and %o virtual regions overlap\n", r, r_prev);
    else if (r.va_start < r_prev.va_start)
      fail("! %o's region comes after a later region %o\n", r, r_prev);

    debug("map [%s] %p -> %p\n", VMRegionTag_names[i], r.va_start, r.va_end);
    vmm_ptable_map(table, r);
  }
}

/** allocate entries which are shared for all translation tables
 * which is the 1G block that maps the IO region 0x0000'0000 -> 0x4000'0000
 * and the 1G table that maps 0x4000'0000 -> TOP_OF_HEAP
 */
static void __vm_alloc_shared_2g_region(u64* root_pgtable) {
  VMRegions map = {
    .regions = {
      /* virt Memory Mapped I/O
      * 0x00000000 -> 0x08000000  == Boot ROM
      * 0x08000000 -> 0x09000000  == GIC
      * 0x09000000 -> 0x09001000  == UART
      * ....       -> 0x40000000  == MMIO/PCIE/etc
      *
      * see https://github.com/qemu/qemu/blob/master/hw/arm/virt.c
      *
      * RPi 3 (BCM2837) Mapped I/O
      * 0x00000000 -> 0x3F000000   == ???
      * 0x3F000000 -> 0x40000000   == peripherals
      * ...
      *
      * Actual RAM is from
      * 0x40000000 -> RAM_END
      *  where RAM_END is defined by the dtb
      */
      [VM_MMAP_IO] = { VMREGION_VALID, BOT_OF_IO, TOP_OF_IO, PROT_MEMTYPE_DEVICE, PROT_RW_RW, },
      /* linker .text section
      * this contains the harness, litmus and unittest code segments as well
      * as the initial boot segment that occurs before BOT_OF_TEXT
      * executable code must be non-writable at EL0 if executable at EL1
      */
      [VM_TEXT] = { VMREGION_VALID, BOT_OF_TEXT, TOP_OF_TEXT, PROT_MEMTYPE_NORMAL, PROT_RX_RX, },
      /* data sections contain all static storage duration constants
      */
      [VM_DATA] = { VMREGION_VALID, BOT_OF_DATA, TOP_OF_DATA, PROT_MEMTYPE_NORMAL, PROT_RW_RWX, },
      /* physical stack space is mapped by all threads -- but at runtime should use the virtual addr instead
      * if we're allocating a pagetable to be shared between test threads then we map the entire 2M stack space
      */
      [VM_STACK] = { VMREGION_VALID, BOT_OF_STACK_PA, TOP_OF_STACK_PA, PROT_MEMTYPE_NORMAL, PROT_RW_RWX, },
      /* for pagetable allocations at runtime
      * they get contiguously allocated in this region
      */
      [VM_PTABLES] = { VMREGION_VALID, BOT_OF_PTABLES, TOP_OF_PTABLES, PROT_MEMTYPE_NORMAL, PROT_RW_RWX, },
      /* for various non-pagetable run-time allocations
      * we have a heap that ALLOC() makes use of
      */
      [VM_HEAP] = { VMREGION_VALID, BOT_OF_HEAP, TOP_OF_HEAP, PROT_MEMTYPE_NORMAL, PROT_RW_RWX, },
    },
  };

  update_table_from_vmregion_map(root_pgtable, map);
  TRACE_PTABLE("allocated map for %p\n", root_pgtable);
}

static u64* __vm_alloc_base_map(void) {
  u64* root_ptable = zalloc_ptable();
  __vm_alloc_shared_2g_region(root_ptable);
  TRACE_PTABLE("allocated shared 2g @ %p\n", root_ptable);

  VMRegions extra_universal = { {
    /* each map also contains the testdata physical address space itself is mapped in the virtual space
    * this is for tests that might want to have a INIT_IDENTITY_MAP(x)
    */
    [VM_TESTDATA] = { VMREGION_VALID, BOT_OF_TESTDATA, TOP_OF_TESTDATA, PROT_MEMTYPE_NORMAL, PROT_RW_RWX, },
  } };

  update_table_from_vmregion_map(root_ptable, extra_universal);

  return root_ptable;
}

static u64* __vmm_alloc_table(u8 is_test) {
  u64* root_ptable = __vm_alloc_base_map();
  TRACE_PTABLE("allocated base @ %p\n", root_ptable);
  int cpu = get_cpu();

  if (is_test) {
    /* if in a test, then all threads share the same translations
     * so we must map the entire stack and vtable region
     */
    VMRegions test = {
      .regions={
        /* this is for EL0 and EL1 for all threads */
        [VM_MMAP_STACK_EL0] = {
          VMREGION_VALID,
          STACK_MMAP_BASE,
          STACK_MMAP_BASE + 1 * GiB,
          PROT_MEMTYPE_NORMAL,
          PROT_RW_RW,
          BOT_OF_STACK_PA,
        },

        /* VTABLE W+R mapping for all threads */
        [VM_MMAP_VTABLE] = {
          VMREGION_VALID,
          VTABLE_MMAP_BASE,
          VTABLE_MMAP_BASE + 4 * NO_CPUS * KiB,
          PROT_MEMTYPE_NORMAL,
          PROT_RW_RW,
          vector_base_pa,
        },
      },
    };

    update_table_from_vmregion_map(root_ptable, test);
    TRACE_PTABLE("updated test @ %p\n", root_ptable);
  } else {
    u64 vtable_bot, vtable_top, vtable_pa;
    vtable_bot = (u64)THR_VTABLE_VA(cpu);
    vtable_top = (u64)THR_VTABLE_VA(cpu) + PAGE_SIZE;
    vtable_pa = (u64)THR_VTABLE_PA(cpu);

    /* ideally EL0 would be R_RW permissions but AArch64 doesn't allow W at EL0 without W at EL1 */
    VMRegions per_thread_data = {
      .regions={
        [VM_MMAP_STACK_EL0] = {
          VMREGION_VALID,
          STACK_MMAP_THREAD_BOT_EL0(cpu),
          STACK_MMAP_THREAD_TOP_EL0(cpu),
          PROT_MEMTYPE_NORMAL,
          PROT_RW_RW,
          STACK_PYS_THREAD_BOT_EL0(cpu),
        },
        [VM_MMAP_STACK_EL1] = {
          VMREGION_VALID,
          STACK_MMAP_THREAD_BOT_EL1(cpu),
          STACK_MMAP_THREAD_TOP_EL1(cpu),
          PROT_MEMTYPE_NORMAL,
          PROT_RW_U,
          STACK_PYS_THREAD_BOT_EL1(cpu),
        },
        [VM_MMAP_VTABLE] = {
          VMREGION_VALID, vtable_bot, vtable_top, PROT_MEMTYPE_NORMAL, PROT_RW_RW, vtable_pa,
        },
      },
    };

    update_table_from_vmregion_map(root_ptable, per_thread_data);
    TRACE_PTABLE("updated per_thread_data @ %p\n", root_ptable);
  }

  VMRegions harness = {
    .regions={
      /* the harness itself maps all of memory starting @ 64G
      */
      [VM_MMAP_HARNESS] = {
        VMREGION_VALID,
        HARNESS_MMAP_BASE,
        HARNESS_MMAP_BASE + TOTAL_MEM,
        PROT_MEMTYPE_NORMAL,
        PROT_RW_RWX,
        BOT_OF_MEM,
        },
    },
  };

  update_table_from_vmregion_map(root_ptable, harness);
  TRACE_PTABLE("updated harness @ %p\n", root_ptable);

  if (DEBUG && DEBUG_PTABLE_DUMP_TABLES) {
    debug("table:\n");
    vmm_dump_table(root_ptable);
  }

  /* finally, the test data (variables etc) are placed into
   * physical address space TOP_OF_HEAP -> TOP_OF_MEM but with
   * a virtual address space placed after RAM_END
   *
   * we allocate 16 x 8M contiguous regions
   * but spread evenly throughout a 64G VA space located at 128G
   *
   * we don't actually allocate it here
   * but rather allow vmm_ensure_level calls in the per-ASID test setup
   * ensure enough of it is allocated for the given test run.
   */
#if 0
  for (int i = 0 ; i < NR_REGIONS; i++) {
    u64 start_va = TESTDATA_MMAP_8M_VA_FROM_INDEX(i);
    u64 start_pa = TESTDATA_MMAP_8M_PA_FROM_INDEX(i);
    ptable_map_range(root_ptable, start_pa, start_va, start_va + REGION_SIZE, PROT_MEMTYPE_NORMAL | PROT_RW_RWX);
  }
#endif

  return root_ptable;
}

u64* vmm_alloc_new_4k_pgtable(void) {
  u64* ptable = __vmm_alloc_table(0);
  debug("allocated new generic 4k pgtable rooted at %p\n", ptable);
  DEBUG(DEBUG_PTABLE, "ptable @ %p has %ld nested tables\n", ptable, vmm_count_subtables(ptable));
  DEBUG(
    DEBUG_PTABLE && DEBUG_ALLOCS,
    "and is now using %ld/%ld alloc chunks\n",
    valloc_alloclist_count_chunks(),
    NUM_ALLOC_CHUNKS
  );
  return ptable;
}

u64* vmm_alloc_new_test_pgtable(void) {
  u64* ptable = __vmm_alloc_table(1);
  debug("allocated new test pgtable rooted at %p\n", ptable);
  DEBUG(DEBUG_PTABLE, "ptable @ %p has %ld nested tables\n", ptable, vmm_count_subtables(ptable));
  return ptable;
}

static void set_new_ttable(u64 ttbr, u64 tcr, u64 mair) {
  /* no printf happens here, so need to worry about disabling locking during them */
  if ((read_sysreg(sctlr_el1) & 1) == 1) {
    fail("set_new_ttable:  MMU already on!\n");
  }
  write_sysreg(ttbr, ttbr0_el1);
  write_sysreg(tcr, tcr_el1);
  write_sysreg(mair, mair_el1);
  dsb();
  isb();
  vmm_mmu_on();
}

void vmm_set_id_translation(u64* pgtable) {
  if (pgtable == NULL) {
    vmm_mmu_off();
    return;
  }

  /* now set the new TTBR and TCR */
  u64 asid = 0;
  u64 ttbr = TTBR0(pgtable, asid);

  /* clang-format off */
  u64 tcr = \
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
  u64 mair = \
    MAIR_ATTR(PROT_ATTR_DEVICE_nGnRnE, MAIR_DEVICE_nGnRnE)  |
    MAIR_ATTR(PROT_ATTR_DEVICE_GRE, MAIR_DEVICE_GRE)        |
    MAIR_ATTR(PROT_ATTR_NORMAL_NC, MAIR_NORMAL_NC)          |
    MAIR_ATTR(PROT_ATTR_NORMAL_RA_WA, MAIR_NORMAL_RA_WA)    |
    0;
  /* clang-format on */

  set_new_ttable(ttbr, tcr, mair);
}

void vmm_switch_ttable(u64* new_table) {
  write_sysreg(TTBR0(new_table, 0), ttbr0_el1);
  dsb();
  isb();
  vmm_flush_tlb();
}

void vmm_switch_asid(u64 asid) {
  u64 ttbr = read_sysreg(ttbr0_el1);
  write_sysreg(TTBR0(ttbr, asid), ttbr0_el1);
  isb(); /* is this needed? */
}

void vmm_switch_ttable_asid(u64* new_table, u64 asid) {
  write_sysreg(TTBR0(new_table, asid), ttbr0_el1);
  isb();
}

static void _vmm_walk_table(u64* pgtable, int level, u64 va_start, walker_cb_t* cb_f, void* data) {
  for (int i = 0; i < 512; i++) {
    u64 va_start_i = va_start + i * LEVEL_SIZES[level];
    desc_t d = read_desc(pgtable[i], level);
    if (d.type == Table) {
      _vmm_walk_table((u64*)d.table_addr, level + 1, va_start_i, cb_f, data);
    }

    int is_leaf = d.type == Table ? 0 : 1;
    va_range r = { va_start_i, va_start_i + LEVEL_SIZES[level] };
    cb_f(pgtable, level, i, &pgtable[i], d, is_leaf, r, data);
  }
}

void vmm_walk_table(u64* root, walker_cb_t* cb_f, void* data) {
  _vmm_walk_table(root, 0, 0, cb_f, data);
}

static void __vmm_free_pgtable(u64* pgtable, int level) {
  fail("cannot free a pgtable, must restore valloc_ptable_restore back to an old state.\n");
}

void vmm_free_generic_pgtable(u64* pgtable) {
  debug("free generic pgtable @ %p\n", pgtable);
  __vmm_free_pgtable(pgtable, 0);
}

void vmm_free_test_pgtable(u64* pgtable) {
  debug("free test pgtable @ %p\n", pgtable);
  __vmm_free_pgtable(pgtable, 0);
}