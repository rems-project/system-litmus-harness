#ifndef VMM_H
#define VMM_H
#include <stdint.h>

#include "vmm_types.h"
#include "vmm_tables.h"
#include "vmm_prot.h"
#include "vmm_va_macros.h"
#include "vmm_regions.h"
#include "sync.h"

/* global pagetable */
extern uint64_t** vmm_pgtables;

/* translation functions */

/* given a VA return a pointer to the last-level pagetable (descriptor) entry
 * that maps that VA
 */
uint64_t* vmm_pte(uint64_t* root, uint64_t va);

/** given a VA, returns a pointer to the desc_level entry
 * that translate VA in the root translation table
 *
 * ensuring that a block translation exists at level ensure_level
 */
uint64_t* vmm_desc_at_level(uint64_t* root, uint64_t va, int ensure_level, int desc_level);

/** given a VA return a pointer to the given level's entry (or the first descriptor,
 * whichever is sooner in the translation) that maps that VA.
 */
uint64_t* vmm_pte_at_level(uint64_t* root, uint64_t va, int level);

/* given a VA return the PA by performing a software translation table walk
 */
uint64_t* vmm_pa(uint64_t* root, uint64_t va);

/* returns 1 if the given addr is valid */
int vmm_pte_valid(uint64_t* root, uint64_t* addr);

/* ensure that the last-level entry for a given virtual address is at some desired level */
void vmm_ensure_level(uint64_t* root, int desired_level, uint64_t va);

uint64_t* vmm_block(uint64_t* root, uint64_t va);
int vmm_level(uint64_t* root, uint64_t va);

desc_t vmm_translation_walk(uint64_t* root, uint64_t va);

/** create a descriptor that maps to the given PA with the given protections
 * as an entry in a level 'level' table */
uint64_t vmm_make_desc(uint64_t pa, uint64_t prot, int level);

/** given a va perform a translation table walk
 * and gather the attrs
 */
attrs_t vmm_read_attrs(uint64_t* root, uint64_t va);

/** helpers for defining pagetable mappings
 */

typedef enum {
  VM_MMAP_IO,
  VM_TEXT,
  VM_DATA,
  VM_STACK,
  VM_HEAP,
  VM_TESTDATA,
  VM_MMAP_HARNESS,
  VM_MMAP_STACK_EL0,
  VM_MMAP_STACK_EL1,
  VM_MMAP_VTABLE,
} VMRegionTag;

typedef enum {
  VMREGION_NOT_SET = 0,
  VMREGION_VALID = 1
} VMRegionValid;

typedef struct {
  uint8_t valid;
  uint64_t va_start;
  uint64_t va_end;
  uint64_t memattr;
  uint64_t prot;
  uint64_t pa_start;
} VMRegion;

typedef struct {
  VMRegion regions[1+VM_MMAP_VTABLE];
} VMRegions;


#define __CHECK(str_start, start, str_end, end, str_expected, expected) \
  ({ \
    if (end - start != expected) { \
      fail( \
        "! __CHECK: from %s=%p to %s=%p was not %s=0x%lx bytes !\n", \
        str_start, \
        start, \
        str_end, \
        end, \
        str_expected , \
        expected \
      ); \
    } \
    start; \
  }) \

#define __CHECK_WITHALIGN(str_start, start, str_end, end, str_expected, expected) \
  ({ \
    if (! IS_ALIGNED_TO(start, expected)) { \
      fail( \
        "! __CHECK_WITHALIGN: %s=%p was not aligned to %s=0x%lx bytes !\n", \
        str_start, \
        start, \
        str_expected , \
        expected \
      ); \
    } \
    __CHECK(str_start, start, str_end, end, str_expected, expected); \
  }) \

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
void vmm_ptable_map(uint64_t* pgtable, VMRegion reg);

/** unmap a VMRegion
 */
void vmm_ptable_unmap(uint64_t* pgtable, VMRegion reg);

/** create a new translation table to be used by the harness itself
 */
uint64_t* vmm_alloc_new_4k_pgtable(void);

/** create a new translation table to be shared by test threads during test
 * execution
 */
uint64_t* vmm_alloc_new_test_pgtable(void);

/** free a given pagetable */
void vmm_free_generic_pgtable(uint64_t* root);
void vmm_free_test_pgtable(uint64_t* root);

typedef struct {
  uint64_t start;
  uint64_t end;
} va_range;

typedef void walker_cb_t(uint64_t* parent_table, int level, int index, uint64_t* entry, desc_t desc, int is_leaf, va_range range, void* data);
void vmm_walk_table(uint64_t* root, walker_cb_t* cb_f, void* data);

/* MMU control */
void vmm_mmu_off(void);
void vmm_mmu_on(void);

/* switch between virtual stack space and physical stack space
 * for this current CPU */
void switch_to_vm_stack(void);
void switch_to_pys_stack(void);

#define MMU_ON thread_infos[get_cpu()].mmu_enabled
#define MMU_OFF (!thread_infos[get_cpu()].mmu_enabled)

void vmm_set_id_translation(uint64_t* pgtable);
void vmm_switch_ttable(uint64_t* new_table);
void vmm_switch_asid(uint64_t asid);
void vmm_switch_ttable_asid(uint64_t* new_table, uint64_t asid);

/* unsynchronized TLB flushes */
void tlbi_va(uint64_t va);
void tlbi_asid(uint64_t asid);
void tlbi_all(void);

/* synchronized TLB flushes */
void vmm_flush_tlb_vaddr(uint64_t va);
void vmm_flush_tlb(void);
void vmm_flush_tlb_asid(uint64_t asid);


/* for debugging and serializing */

/** given a translation table
 * walk the table and dump all the entries */
void vmm_dump_table(uint64_t* ptable);

/* count the number of other pagetables underneath this one
 */
uint64_t vmm_count_subtables(uint64_t* ptable);

#endif /* VMM_H */
