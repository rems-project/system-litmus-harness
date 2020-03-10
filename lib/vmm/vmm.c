#include "lib.h"

uint64_t* vmm_pgtable;

void vmm_ensure_level(uint64_t* root, int desired_level, uint64_t va) {
  lock(&vmm_lock);

  desc_t block_desc = { .level = 0 };

  for (int level = 0; level <= desired_level - 1; level++) {
    uint64_t* p = root + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    desc_t new_desc;

    if (desc.type == Table) {
      root = (uint64_t*)desc.table_addr;
      continue;
    }

    uint64_t* pg = alloc(4096);
    valloc_memset(pg, 0, 4096);
    for (int i = 0; i < 512; i++) {
      switch (desc.type) {
        case Invalid:
          pg[i] = write_desc(desc);
          break;

        case Block:
          if (block_desc.level == 0)
            block_desc = desc;

          new_desc.type = Block;
          new_desc.oa = desc.oa + (i << OFFSBOT(level+1));
          new_desc.level = level+1;
          new_desc.attrs = block_desc.attrs;
          pg[i] = write_desc(new_desc);
          break;

        default:
          unreachable();
      }
    }

    new_desc.type = Table;
    new_desc.level = level;
    new_desc.table_addr = (uint64_t)pg;
    new_desc.attrs = (attrs_t){0};
    *p = write_desc(new_desc);
    root = pg;
  }
  unlock(&vmm_lock);
}

desc_t read_descptr(uint64_t* desc, int level) {
  return read_desc(*desc, level);
}

desc_t vmm_translation_walk(uint64_t* root, uint64_t va) {
  lock(&vmm_lock);
  desc_t ret;

  for (int level = 0; level <= 3; level++) {
    uint64_t* p = root + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    if (desc.type == Invalid || desc.type == Block) {
      ret = desc;
      ret.src = p;
      goto vmm_translation_walk_end;
    }

    root = (uint64_t*)desc.table_addr;
  }

vmm_translation_walk_end:
  unlock(&vmm_lock);
  return ret;
}

uint64_t* vmm_block(uint64_t* root, uint64_t va) {
  desc_t desc = vmm_translation_walk(root, va);
  return desc.src;
}

int vmm_level(uint64_t* root, uint64_t va) {
  desc_t desc = vmm_translation_walk(root, va);
  return desc.level;
}

uint64_t* vmm_pte(uint64_t* root, uint64_t va) {
  vmm_ensure_level(root, 3, va);
  return vmm_block(root, va);
}

uint64_t* vmm_pa(uint64_t* root, uint64_t va) {
  uint64_t* ret;
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
  uint64_t* pte = vmm_pte(root, (uint64_t)addr);
  if ((*pte & 1) == 0) {
    return 0;
  }

  return 1;
}