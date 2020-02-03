#include "lib.h"

void cpu_data_init(void) {
    for (int i = 0; i < 4; i++) {
        cpu_data[i].started = 0;
        cpu_data[i].to_execute = 0;
        cpu_data[i].lock = 1;
    }
}

void cpu_boot(uint64_t cpu) {
    psci_cpu_on(cpu);
    while (!cpu_data[cpu].started) wfe();
}

void run_on_cpu(uint64_t cpu, async_fn_t* fn, void* arg) {
    while (!cpu_data[cpu].started);

    uint64_t cur_cpu = get_cpu();

    if (cur_cpu == cpu) {
        fn(cpu, arg);
        return;
    }

    lock(&cpu_data[cpu].lock);
    cpu_data[cpu].arg = arg;
    dmb();
    cpu_data[cpu].to_execute = fn;
}

int secondary_init(int cpu) {
    /* switch on MMU and point to id-mapped pagetable */
    vmm_set_new_id_translation(vmm_pgt_idmap);

    cpu_data[cpu].to_execute = 0;
    dmb();
    cpu_data[cpu].started = 1;
    unlock(&cpu_data[cpu].lock);

    return cpu;
}

void secondary_idle_loop(int cpu) {
    while (1) {
        if (cpu_data[cpu].to_execute != 0) {
            async_fn_t* fn = (async_fn_t*)cpu_data[cpu].to_execute;
            cpu_data[cpu].to_execute = 0;
            fn(cpu, cpu_data[cpu].arg);
            unlock(&cpu_data[cpu].lock);
        } else {
            wfe();
        }
    }
}