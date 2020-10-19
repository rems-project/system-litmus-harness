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

uint64_t* vmm_alloc_new_4k_pgtable(void);
uint64_t* vmm_alloc_new_test_pgtable(void);
void vmm_free_pgtable(uint64_t* root);
void vmm_mmu_off(void);
void vmm_mmu_on(void);

#define MMU_ON thread_infos[get_cpu()].mmu_enabled
#define MMU_OFF (!thread_infos[get_cpu()].mmu_enabled)

void vmm_set_id_translation(uint64_t* pgtable);
void vmm_update_mapping(uint64_t* pgtable, uint64_t va, uint64_t pa, uint64_t prot);
void vmm_unmap_page(uint64_t* pgtable, uint64_t va);
void vmm_switch_ttable(uint64_t* new_table);
void vmm_switch_asid(uint64_t asid);

/* unsynchronized TLB flushes */
void tlbi_va(uint64_t va);
void tlbi_asid(uint64_t asid);
void tlbi_all(void);

/* synchronized TLB flushes */
void vmm_flush_tlb_vaddr(uint64_t va);
void vmm_flush_tlb(void);
void vmm_flush_tlb_asid(uint64_t asid);
#endif /* VMM_H */
