#ifndef THREAD_INFO_H
#define THREAD_INFO_H

#include <stdint.h>

typedef struct {
    /** which physical CPU this is on */
    uint64_t cpu_no;

    /** which virtual CPU (thread no) this currently is (P0, P1, etc) */
    uint64_t vcpu_no;

    uint8_t mmu_enabled;
    uint8_t locking_enabled;
} thread_info_t;

extern thread_info_t thread_infos[4];

uint64_t get_cpu(void);
uint64_t get_vcpu(void);

/** set the current vcpu for this thread */
void set_vcpu(int vcpu);

thread_info_t* current_thread_info();

#endif /* THREAD_INFO_H */