#include "lib.h"

void cpu_boot(uint64_t cpu) {
    psci_cpu_on(cpu);
    while (!cpu_data[cpu].started) wfe();
}

void run_on_cpu(uint64_t cpu, async_fn_t* fn) {
    while (!cpu_data[cpu].started);
    cpu_data[cpu].to_execute = fn;
}

int secondary_init(int cpu) {
    cpu_data[cpu].to_execute = 0;
    dmb();
    cpu_data[cpu].started = 1;

    return cpu;
}

void secondary_idle_loop(int cpu) {
    while (1) {
        if (cpu_data[cpu].to_execute != 0) {
            cpu_data[cpu].to_execute = 0;
            cpu_data[cpu].to_execute(cpu);
        } else {
            wfe();
        }
    }
}