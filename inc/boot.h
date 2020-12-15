#ifndef BOOT_H
#define BOOT_H

typedef enum {
  BOOT_KIND_PSCI,
  BOOT_KIND_SPIN
} boot_kind_t;

typedef struct {
  boot_kind_t kind;
  union {
    uint64_t psci_base;
    uint64_t spin_base[4];
  };
} boot_data_t;

extern boot_data_t boot_data;

/* spintable booter */
void spin_cpu_on(uint64_t cpu);

/* psci booter */
void psci_cpu_on(uint64_t cpu);
void psci_system_off(void);

#endif /* BOOT_H */
