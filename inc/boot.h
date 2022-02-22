#ifndef BOOT_H
#define BOOT_H

typedef enum {
  BOOT_KIND_PSCI,
  BOOT_KIND_SPIN
} boot_kind_t;

typedef struct {
  boot_kind_t kind;
  union {
    u64 psci_base;
    u64 spin_base[4];
  };
} boot_data_t;

extern boot_data_t boot_data;

/* spintable booter */
void spin_cpu_on(u64 cpu);

/* psci booter */
void psci_cpu_on(u64 cpu);
void psci_system_off(void);

#endif /* BOOT_H */
