#include <lib.h>

#include "vmm.h"

void setup(char* fdtloc) {
    fdt = fdtloc;

    init_device();
    init_valloc();

    /* read passed args */
    init_args();

    vector_base_addr = (uint64_t)&el1_exception_vector_table_p0;

    /* create pgtable */
    if (ENABLE_PGTABLE) {
        vmm_pgtable = vmm_alloc_new_idmap_4k();
        vmm_set_id_translation(vmm_pgtable);
    }

    cpu_data_init();
    cpu_data[0].started = 1;
}