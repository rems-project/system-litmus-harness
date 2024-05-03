#ifndef THREAD_INFO_H
#define THREAD_INFO_H

typedef struct
{
  /** which physical CPU this is on */
  u64 cpu_no;

  /** which virtual CPU (thread no) this currently is (P0, P1, etc) */
  u64 vcpu_no;

  u8 mmu_enabled;
  u8 locking_enabled;
} thread_info_t;

extern thread_info_t thread_infos[4];

u64 get_cpu(void);
u64 get_vcpu(void);

/** set the current vcpu for this thread */
void set_vcpu(int vcpu);

thread_info_t* current_thread_info();

#endif /* THREAD_INFO_H */