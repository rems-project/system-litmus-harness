#include "lib.h"

/** each hardware thread
 * has its own pagetable
 * for running the harness in
 *
 * note that there is a singular global
 * *test* translation table stored in the test_ctx_t
 */

uint64_t** vmm_pgtables;

lock_t _vmm_lock;

void vmm_ensure_level(uint64_t* root, int desired_level, uint64_t va) {
  desc_t block_desc = { .level = 0 };

  /** since multiple threads might want to vmm_ensure_level
   * at the same time, we synchronize them
   */
  LOCK(&_vmm_lock);

  DEBUG(DEBUG_TRACE_VMM_ENSURES, "ensuring va=%p to level %d for pgtable %p\n", va, desired_level, root);

  uint64_t* current = root;

  for (int level = 0; level <= desired_level - 1; level++) {
    uint64_t* p = current + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    DEBUG(DEBUG_TRACE_VMM_ENSURES, ".. at level %d, entry=%p desc=0x%lx\n", level, p, *p);
    desc_t new_desc;

    if (desc.type == Table) {
      current = (uint64_t*)desc.table_addr;
      continue;
    }

    uint64_t* pg = alloc(4096);

    if (level == desired_level - 1) {
      debug("ensuring new entry for %p at level %d in pagetable rooted at %p\n", va, desired_level, root);
    }

    DEBUG(DEBUG_TRACE_VMM_ENSURES, "ensure %p to level%d alloc new level%d @ %p\n", va, desired_level, level, pg);
    for (int i = 0; i < 512; i++) {
      switch (desc.type) {
        case Invalid:
          DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback invalid %p = 0x%lx\n", &pg[i], write_desc(new_desc));
          pg[i] = write_desc(desc);
          break;

        case Block:
          if (block_desc.level == 0)
            block_desc = desc;

          new_desc.type = Block;
          new_desc.oa = desc.oa + (i << OFFSBOT(level+1));
          new_desc.level = level+1;
          new_desc.attrs = block_desc.attrs;
          DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback block %p = 0x%lx\n", &pg[i], write_desc(new_desc));

          pg[i] = write_desc(new_desc);
          break;

        case Table:
          unreachable();
      }
    }

    new_desc.type = Table;
    new_desc.level = level;
    new_desc.table_addr = (uint64_t)pg;
    new_desc.attrs = (attrs_t){0};
    DEBUG(DEBUG_TRACE_VMM_ENSURES, "writeback table %p = 0x%lx\n", p, write_desc(new_desc));
    *p = write_desc(new_desc);
    current = pg;
  }

  UNLOCK(&_vmm_lock);
}

desc_t read_descptr(uint64_t* desc, int level) {
  return read_desc(*desc, level);
}

desc_t vmm_translation_walk_to_level(uint64_t* root, uint64_t va, int max_level) {
  desc_t desc;

  uint64_t* parent = NULL;

  for (int level = 0; level <= max_level; level++) {
    uint64_t* p = root + OFFS(va, level);
    uint64_t* parents[4];
    valloc_memcpy(parents, desc.parents, 4*sizeof(uint64_t*));

    desc = read_desc(*p, level);
    desc.src = p;
    valloc_memcpy(desc.parents, parents, 4*sizeof(uint64_t*));
    desc.parents[level] = parent;

    if ((desc.type == Invalid) || (desc.type == Block)) {
      goto vmm_translation_walk_end;
    }

    root = (uint64_t*)desc.table_addr;
    parent = p;
  }

vmm_translation_walk_end:
  return desc;
}

desc_t vmm_translation_walk(uint64_t* root, uint64_t va) {
  return vmm_translation_walk_to_level(root, va, 3);
}

uint64_t* vmm_block(uint64_t* root, uint64_t va) {
  desc_t desc = vmm_translation_walk(root, va);
  return desc.src;
}

int vmm_level(uint64_t* root, uint64_t va) {
  desc_t desc = vmm_translation_walk(root, va);
  return desc.level;
}

uint64_t* vmm_desc_at_level(uint64_t* root, uint64_t va, int ensure_level, int desc_level) {
  vmm_ensure_level(root, ensure_level, va);
  desc_t desc = vmm_translation_walk_to_level(root, va, desc_level);
  return desc.src;
}

uint64_t* vmm_pte_at_level(uint64_t* root, uint64_t va, int level) {
  return vmm_desc_at_level(root, va, 3, level);
}

uint64_t* vmm_pte(uint64_t* root, uint64_t va) {
  return vmm_pte_at_level(root, va, 3);
}

attrs_t vmm_read_attrs(uint64_t* root, uint64_t va) {
  /* TODO: collect from all levels not just level 3 */
  uint64_t* pte = vmm_pte(root, va);
  return read_attrs(*pte);
}

uint64_t* vmm_pa(uint64_t* root, uint64_t va) {
  desc_t desc = vmm_translation_walk(root, va);

  switch (desc.type) {
    case Invalid:
      return NULL;

    case Block:
      return (uint64_t*)(desc.oa | BIT_SLICE(va, OFFSBOT(desc.level), 0));

    case Table:
      unreachable();
  }

  return NULL;
}

int vmm_pte_valid(uint64_t* root, uint64_t* addr) {
  if (root == NULL)
    return 1;

  uint64_t* pte = vmm_pte(root, (uint64_t)addr);
  if ((*pte & 1) == 0) {
    return 0;
  }

  return 1;
}