#include <lib.h>

#include "vmm.h"

void setup(void) {
    init_device();
    init_valloc();

    /* create pgtable */
    if (ENABLE_PGTABLE) {
        vmm_pgtable = alloc_new_idmap_4k();
        vmm_set_new_id_translation(vmm_pgtable);
    }

    cpu_data_init();
    cpu_data[0].started = 1;
}
