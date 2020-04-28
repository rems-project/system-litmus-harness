#ifndef VMM_H
#define VMM_H
#include <stdint.h>

#include "vmm_types.h"
#include "vmm_tables.h"
#include "sync.h"

#define PAGE(va) ((uint64_t)(va)>>12)

/* global pagetable */
extern uint64_t* vmm_pgtable;

/* translation functions */

/* given a VA return a pointer to the last-level pagetable (descriptor) entry
 * that maps that VA
 */
uint64_t* vmm_pte(uint64_t* root, uint64_t va);

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

uint64_t* vmm_alloc_new_idmap_4k(void);
void vmm_free_pgtable(uint64_t* root);
void vmm_mmu_off(void);
void vmm_mmu_on(void);

void vmm_set_id_translation(uint64_t* pgtable);
void vmm_update_mapping(uint64_t* pgtable, uint64_t va, uint64_t pa, uint64_t prot);
void vmm_switch_ttable(uint64_t* new_table);

void vmm_flush_tlb_vaddr(uint64_t va);
void vmm_flush_tlb(void);
#endif /* VMM_H */
