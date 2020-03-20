#include "lib.h"

#include "vmm.h"

thread_info_t thread_infos[4];

thread_info_t* current_thread_info() {
  return &thread_infos[get_cpu()];
}

void cpu_data_init(void) {
  for (int i = 0; i < 4; i++) {
    cpu_data[i].started = 0;
    cpu_data[i].to_execute = 0;
  }
}

void cpu_boot(uint64_t cpu) {
  psci_cpu_on(cpu);
  while (!cpu_data[cpu].started) wfe();
}

void run_on_cpu_async(uint64_t cpu, async_fn_t* fn, void* arg) {
  while (!cpu_data[cpu].started) wfe();

  cpu_data[cpu].arg = arg;
  dmb();
  cpu_data[cpu].to_execute = fn;
  dsb();
  sev();
}

void run_on_cpu(uint64_t cpu, async_fn_t* fn, void* arg) {
  int count = cpu_data[cpu].count;
  run_on_cpu_async(cpu, fn, arg);
  while (cpu_data[cpu].count == count) wfe();
}

void run_on_cpus(async_fn_t* fn, void* arg) {
  uint64_t cur_cpu = get_cpu();
  
  for (int i = 0; i < 4; i++)
    while (!cpu_data[i].started) wfe();

  int counts[4] = { 0 };
  for (int i = 0; i < 4; i++) {
    counts[i] = cpu_data[i].count;
    if (i != cur_cpu) {
      run_on_cpu_async(i, fn, arg);
    }
  }

  fn(cur_cpu, arg);
  cpu_data[cur_cpu].count++;
  sev();

  for (int i = 0; i < 4; i++) {
    while (cpu_data[i].count == counts[i]) wfe();
  }
}

int secondary_init(int cpu) {
  if (ENABLE_PGTABLE) {
    vmm_set_id_translation(vmm_pgtable);
  }

  current_thread_info()->printer_lock_enabled = 1;

  cpu_data[cpu].to_execute = 0;
  dmb();
  cpu_data[cpu].started = 1;

  return cpu;
}

void secondary_idle_loop(int cpu) {
  while (1) {
    if (cpu_data[cpu].to_execute != 0) {
      async_fn_t* fn = (async_fn_t*)cpu_data[cpu].to_execute;
      cpu_data[cpu].to_execute = 0;
      fn(cpu, cpu_data[cpu].arg);
      cpu_data[cpu].count++;
    } else {
      wfe();
    }
  }
}
