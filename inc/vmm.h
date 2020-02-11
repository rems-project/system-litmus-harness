#include <stdint.h>

/* translation functions */

/* given a VA return a pointer to the last-level pagetable (descriptor) entry
 * that maps that VA
 */
uint64_t* vmm_pte(uint64_t* root, uint64_t va);

/* given a VA return the PA by performing a software translation table walk
 */
uint64_t* vmm_pa(uint64_t* root, uint64_t va);


uint64_t* alloc_new_idmap_4k(void);
void vmm_mmu_off(void);

void vmm_set_new_id_translation(uint64_t* pgtable);
void vmm_restore_old_id_translation(uint64_t ttbr, uint64_t tcr);

void vmm_flush_tlb_vaddr(uint64_t va);
void vmm_flush_tlb(void);