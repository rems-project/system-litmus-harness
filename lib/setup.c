#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

void setup(char* fdtloc) {
    fdt = fdtloc;

    current_thread_info()->mmu_enabled = 0;
    current_thread_info()->printer_lock_enabled = 0;

    init_device();
    init_valloc();

    /* read passed args */
    init_args(); 
    read_args(__argc, __argv);

    vector_base_addr = (uint64_t)&el1_exception_vector_table_p0;

    /* create pgtable */
    if (ENABLE_PGTABLE) {
        vmm_pgtable = vmm_alloc_new_idmap_4k();
        vmm_set_id_translation(vmm_pgtable);
    }

    current_thread_info()->printer_lock_enabled = 1;

    cpu_data_init();
    cpu_data[0].started = 1;
}