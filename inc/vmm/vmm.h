#ifndef VMM_H
#define VMM_H

#include "vmm_types.h"
#include "vmm_tables.h"
#include "vmm_prot.h"
#include "vmm_va_macros.h"
#include "vmm_regions.h"
#include "sync.h"

/* global pagetable */
extern u64** vmm_pgtables;

/* translation functions */

/* given a VA return a pointer to the last-level pagetable (descriptor) entry
 * that maps that VA
 */
u64* vmm_pte(u64* root, u64 va);

/** given a VA, returns a pointer to the desc_level entry
 * that translate VA in the root translation table
 *
 * ensuring that a block translation exists at level ensure_level
 */
u64* vmm_desc_at_level(u64* root, u64 va, int ensure_level, int desc_level);

/** given a VA return a pointer to the given level's entry (or the first descriptor,
 * whichever is sooner in the translation) that maps that VA.
 */
u64* vmm_pte_at_level(u64* root, u64 va, int level);

/* given a VA return the PA by performing a software translation table walk
 */
u64* vmm_pa(u64* root, u64 va);

/* returns 1 if the given addr is valid */
int vmm_pte_valid(u64* root, u64* addr);

/* ensure that the last-level entry for a given virtual address is at some desired level */
void vmm_ensure_level(u64* root, int desired_level, u64 va);

u64* vmm_block(u64* root, u64 va);
int vmm_level(u64* root, u64 va);

desc_t vmm_translation_walk(u64* root, u64 va);

/** create a descriptor that maps to the given PA with the given protections
 * as an entry in a level 'level' table */
u64 vmm_make_desc(u64 pa, u64 prot, int level);

/** given a va perform a translation table walk
 * and gather the attrs
 */
attrs_t vmm_read_attrs(u64* root, u64 va);

/** helpers for defining pagetable mappings
 */

typedef enum {
  VM_MMAP_IO,
  VM_TEXT,
  VM_DATA,
  VM_STACK,
  VM_HEAP,
  VM_PTABLES,
  VM_TESTDATA,
  VM_MMAP_HARNESS,
  VM_MMAP_STACK_EL0,
  VM_MMAP_STACK_EL1,
  VM_MMAP_VTABLE,
} VMRegionTag;

typedef enum { VMREGION_NOT_SET = 0, VMREGION_VALID = 1 } VMRegionValid;

typedef struct
{
  u8 valid;
  u64 va_start;
  u64 va_end;
  u64 memattr;
  u64 prot;
  u64 pa_start;
} VMRegion;

typedef struct
{
  VMRegion regions[1 + VM_MMAP_VTABLE];
} VMRegions;

#define __CHECK(str_start, start, str_end, end, str_expected, expected) \
  ({                                                                    \
    if (end - start != expected) {                                      \
      fail(                                                             \
        "! __CHECK: from %s=%p to %s=%p was not %s=0x%lx bytes !\n",    \
        str_start,                                                      \
        start,                                                          \
        str_end,                                                        \
        end,                                                            \
        str_expected,                                                   \
        expected                                                        \
      );                                                                \
    }                                                                   \
    start;                                                              \
  })

#define __CHECK_WITHALIGN(str_start, start, str_end, end, str_expected, expected)                                    \
  ({                                                                                                                 \
    if (!IS_ALIGNED_TO(start, expected)) {                                                                           \
      fail(                                                                                                          \
        "! __CHECK_WITHALIGN: %s=%p was not aligned to %s=0x%lx bytes !\n", str_start, start, str_expected, expected \
      );                                                                                                             \
    }                                                                                                                \
    __CHECK(str_start, start, str_end, end, str_expected, expected);                                                 \
  })

#define FIXED_ID_REGION(expected_size, start, end, memattr, prot) \
  { __CHECK(#start, start, #end, end, #expected_size, expected_size), end, memattr, prot }

#define FIXED_ALIGNED_ID_REGION(expected_size, start, end, memattr, prot) \
  { __CHECK_WITHALIGN(#start, start, #end, end, #expected_size, expected_size), end, memattr, prot }

#define FIXED_REGION(expected_size, pa_start, start, end, memattr, prot) \
  { __CHECK(#start, start, #end, end, #expected_size, expected_size), end, memattr, prot, pa_start }

#define VARIABLE_ID_REGION(start, end, memattr, prot) \
  { start, end, memattr, prot }

#define VARIABLE_REGION(start, end, memattr, prot, pa_start) \
  { start, end, memattr, prot, pa_start }

/** map a VMRegion
 * creating a new entry in the pagetable
 */
void vmm_ptable_map(u64* pgtable, VMRegion reg);

/** unmap a VMRegion
 */
void vmm_ptable_unmap(u64* pgtable, VMRegion reg);

/** create a new translation table to be used by the harness itself
 */
u64* vmm_alloc_new_4k_pgtable(void);

/** create a new translation table to be shared by test threads during test
 * execution
 */
u64* vmm_alloc_new_test_pgtable(void);

/** free a given pagetable */
void vmm_free_generic_pgtable(u64* root);
void vmm_free_test_pgtable(u64* root);

typedef struct
{
  u64 start;
  u64 end;
} va_range;

typedef void
walker_cb_t(u64* parent_table, int level, int index, u64* entry, desc_t desc, int is_leaf, va_range range, void* data);
void vmm_walk_table(u64* root, walker_cb_t* cb_f, void* data);

/* MMU control */
void vmm_mmu_off(void);
void vmm_mmu_on(void);
void vmm_ensure_in_harness_pgtable_ctx(void);

/* switch between virtual stack space and physical stack space
 * for this current CPU */
bool is_using_vm_stack(void);
void switch_to_vm_stack(void);
void switch_to_pys_stack(void);

#define MMU_ON thread_infos[get_cpu()].mmu_enabled
#define MMU_OFF (!thread_infos[get_cpu()].mmu_enabled)

void vmm_set_id_translation(u64* pgtable);
void vmm_switch_ttable(u64* new_table);
void vmm_switch_asid(u64 asid);
void vmm_switch_ttable_asid(u64* new_table, u64 asid);

/* unsynchronized TLB flushes */
void tlbi_va(u64 va);
void tlbi_asid(u64 asid);
void tlbi_all(void);

/* synchronized TLB flushes */
void vmm_flush_tlb_vaddr(u64 va);
void vmm_flush_tlb(void);
void vmm_flush_tlb_asid(u64 asid);

/* synchronised (with BBM) pgtable updates */

/**
 * vmm_update_pte() - Write to a pagetable entry.
 * @pte: location of pagetable entry.
 * @new_val: new 64-bit descriptor to write to that location.
 * @sync_kind: what kind of TLB maintenance to perform (by-ALL, by-ASID, by-VA).
 * @asid_or_va: if sync by-ASID or -VA, the ASID or VA to invalidate for this update.
 * @force: unconditionally perform TLB maintenance on the location.
 *
 * Note: VA should be the full VA, not just the page number.
 */
void vmm_update_pte(u64* pte, u64 new_val, sync_type_t sync_kind, u64 asid_or_va, bool force);

/* for debugging and serializing */

/** given a translation table
 * walk the table and dump all the entries */
void vmm_dump_table(u64* ptable);

/* count the number of other pagetables underneath this one
 */
u64 vmm_count_subtables(u64* ptable);

#endif /* VMM_H */
