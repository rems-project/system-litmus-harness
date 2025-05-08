#include "lib.h"

/** each hardware thread
 * has its own pagetable
 * for running the harness in
 *
 * note that there is a singular global
 * *test* translation table stored in the test_ctx_t
 */

u64** vmm_pgtables;

lock_t _vmm_lock;

void vmm_update_pte(u64* pte, u64 new_val, sync_type_t sync_kind, u64 asid_or_va, bool force) {
  bool needs_tlb_maintenance = force;

  if (*pte > 0) {
    write_release(pte, 0);
    needs_tlb_maintenance = true;
  }

  if (needs_tlb_maintenance) {
    if (sync_kind == SYNC_ALL)
      vmm_flush_tlb();
    else if (sync_kind == SYNC_ASID)
      vmm_flush_tlb_asid(asid_or_va);
    else if (sync_kind == SYNC_VA)
      vmm_flush_tlb_vaddr(asid_or_va);
    else
      dsb();
  }

  write_release(pte, new_val);

  if (cpu_needs_workaround(ERRATA_WORKAROUND_ISB_AFTER_PTE_ST)) {
    dsb();
    isb();
  }
}

void vmm_ensure_level(u64* root, int desired_level, u64 va) {
  desc_t block_desc = { .level = 0 };

  /** since multiple threads might want to vmm_ensure_level
   * at the same time, we synchronize them
   */
  LOCK(&_vmm_lock);

  DEBUG(DEBUG_TRACE_VMM_ENSURES, "ensuring va=%p to level %d for pgtable %p\n", va, desired_level, root);

  u64* current = root;

  for (int level = 0; level <= desired_level - 1; level++) {
    u64* p = current + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    DEBUG(DEBUG_TRACE_VMM_ENSURES, ".. at level %d, entry=%p desc=0x%lx\n", level, p, *p);
    desc_t new_desc;

    if (desc.type == Table) {
      current = (u64*)desc.table_addr;
      continue;
    }

    u64* pg = zalloc_ptable();

    if (level == desired_level - 1) {
      debug("ensuring new entry for %p at level %d in pagetable rooted at %p\n", va, desired_level, root);
    }

    DEBUG(DEBUG_TRACE_VMM_ENSURES, "ensure %p to level%d alloc new level%d @ %p\n", va, desired_level, level, pg);
    for (int i = 0; i < 512; i++) {
      switch (desc.type) {
      case Invalid:
        DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback invalid %p = 0x%lx\n", &pg[i], write_desc(new_desc));
        vmm_update_pte(&pg[i], write_desc(desc), SYNC_ALL, 0, false);
        break;

      case Block:
        if (block_desc.level == 0)
          block_desc = desc;

        new_desc.type = Block;
        new_desc.oa = desc.oa + (i << OFFSBOT(level + 1));
        new_desc.level = level + 1;
        new_desc.attrs = block_desc.attrs;
        DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback block %p = 0x%lx\n", &pg[i], write_desc(new_desc));

        vmm_update_pte(&pg[i], write_desc(desc), SYNC_ALL, 0, false);
        break;

      case Table:
        unreachable();
      }
    }

    new_desc.type = Table;
    new_desc.level = level;
    new_desc.table_addr = (u64)pg;
    new_desc.attrs = (attrs_t){ 0 };
    DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback table %p = 0x%lx\n", p, write_desc(new_desc));
    vmm_update_pte(p, write_desc(new_desc), SYNC_ALL, 0, false);
    current = pg;
  }

  UNLOCK(&_vmm_lock);
}

desc_t read_descptr(u64* desc, int level) {
  return read_desc(*desc, level);
}

desc_t vmm_translation_walk_to_level(u64* root, u64 va, int max_level) {
  desc_t desc;

  u64* parent = NULL;

  for (int level = 0; level <= max_level; level++) {
    u64* p = root + OFFS(va, level);
    u64* parents[4];
    valloc_memcpy(parents, desc.parents, 4 * sizeof(u64*));

    desc = read_desc(*p, level);
    desc.src.has_src = true;
    desc.src.ptr = p;
    valloc_memcpy(desc.parents, parents, 4 * sizeof(u64*));
    desc.parents[level] = parent;

    if ((desc.type == Invalid) || (desc.type == Block)) {
      goto vmm_translation_walk_end;
    }

    root = (u64*)desc.table_addr;
    parent = p;
  }

vmm_translation_walk_end:
  return desc;
}

desc_t vmm_translation_walk(u64* root, u64 va) {
  return vmm_translation_walk_to_level(root, va, 3);
}

u64* vmm_block(u64* root, u64 va) {
  desc_t desc = vmm_translation_walk(root, va);
  fail_on(!desc.src.has_src, "bug: desc should have source");
  return desc.src.ptr;
}

int vmm_level(u64* root, u64 va) {
  desc_t desc = vmm_translation_walk(root, va);
  return desc.level;
}

u64* vmm_desc_at_level(u64* root, u64 va, int ensure_level, int desc_level) {
  vmm_ensure_level(root, ensure_level, va);
  desc_t desc = vmm_translation_walk_to_level(root, va, desc_level);
  fail_on(!desc.src.has_src, "bug: desc should have source");
  return desc.src.ptr;
}

u64* vmm_pte_at_level(u64* root, u64 va, int level) {
  return vmm_desc_at_level(root, va, 3, level);
}

u64* vmm_pte(u64* root, u64 va) {
  return vmm_pte_at_level(root, va, 3);
}

attrs_t vmm_read_attrs(u64* root, u64 va) {
  /* TODO: collect from all levels not just level 3 */
  u64* pte = vmm_pte(root, va);
  return read_attrs(*pte);
}

u64* vmm_pa(u64* root, u64 va) {
  desc_t desc = vmm_translation_walk(root, va);

  switch (desc.type) {
  case Invalid:
    return NULL;

  case Block:
    return (u64*)(desc.oa | BIT_SLICE(va, OFFSBOT(desc.level), 0));

  case Table:
    unreachable();
  }

  return NULL;
}

int vmm_pte_valid(u64* root, u64* addr) {
  if (root == NULL)
    return 1;

  u64* pte = vmm_pte(root, (u64)addr);
  if ((*pte & 1) == 0) {
    return 0;
  }

  return 1;
}