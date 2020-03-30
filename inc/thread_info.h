#ifndef THREAD_INFO_H
#define THREAD_INFO_H

#include <stdint.h>

typedef struct {
    uint64_t cpu_no;
    uint8_t mmu_enabled;
    uint8_t locking_enabled;
} thread_info_t;

extern thread_info_t thread_infos[4];

uint64_t get_cpu(void);

thread_info_t* current_thread_info();

#endif /* THREAD_INFO_H */