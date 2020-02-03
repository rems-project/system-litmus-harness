#include <lib.h>

void setup(void) {
    init_device();
    init_valloc();
    
    /* create new 4k pagetable, install it and switch on MMU */
    uint64_t* root_pgtable = alloc_new_idmap_4k();
    vmm_set_new_id_translation(root_pgtable);
    vmm_pgt_idmap = root_pgtable;

    cpu_data_init();
    cpu_data[0].started = 1;
}
