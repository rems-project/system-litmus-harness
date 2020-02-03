#include <stdint.h>

/* barriers and wrappers */
void wfe(void);
void dsb(void);
void dmb(void);
void isb(void);

/* stores and loads */
void writeb(uint8_t byte, uint64_t addr);

/* register read/write */
#define read_sysreg(r) ({  \
    uint64_t reg;  \
    asm volatile("mrs %[reg], " #r : [reg] "=r" (reg));  \
    reg;  \
})

#define write_sysreg(v, r) do {  \
    asm volatile("msr " #r ", %[reg]" : : [reg] "rZ" (v));  \
} while (0)


uint64_t get_cpu(void);