#pragma once
/* clang-format off */
/* ... manual layout of addresses for readability */

#define bus_to_reg32(p) ((p) - 0x3f000000ULL)

#define UINT32_MAX ((1 << 32) - 1)
// Produce a u32 whose n least significant bits are 1, and all others are 0.
#define ONES_U32(n) (UINT32_MAX >> (32 - (n)))

// Produce a u32 representing a mask of n bits starting at offset off.
#define MASK_U32(off, n) (ONES_U32((n)) << (off))
#define MASK_CLEAR_U32(off, n) ~(MASK_U32(off, n))

// Produce a u32 whose i-th bit is 1, and all other bits are 0.
#define BIT_U32(i) ((1) << (i))


// Registers for the auxiliaries (UART1, SPI1, and SPI2).

// Registers controling the three peripherals.
#define AUX_IRQ     bus_to_reg32(0x7e215000ULL)
#define AUX_ENABLES bus_to_reg32(0x7e215004ULL)

// Bit fields of the AUX_ENABLES register.
#define AUX_ENABLES_BIT_UART1 BIT_U32(0)
#define AUX_ENABLES_BIT_SPI1  BIT_U32(1)
#define AUX_ENABLES_BIT_SPI2  BIT_U32(2)

// Registers for UART1 (mini UART).
#define AUX_MU_IO_REG   bus_to_reg32(0x7e215040ULL)
#define AUX_MU_IER_REG  bus_to_reg32(0x7e215044ULL)
#define AUX_MU_IIR_REG  bus_to_reg32(0x7e215048ULL)
#define AUX_MU_LCR_REG  bus_to_reg32(0x7e21504cULL)
#define AUX_MU_MCR_REG  bus_to_reg32(0x7e215050ULL)
#define AUX_MU_LSR_REG  bus_to_reg32(0x7e215054ULL)
#define AUX_MU_MSR_REG  bus_to_reg32(0x7e215058ULL)
#define AUX_MU_SCRATCH  bus_to_reg32(0x7e21505cULL)
#define AUX_MU_CNTL_REG bus_to_reg32(0x7e215060ULL)
#define AUX_MU_STAT_REG bus_to_reg32(0x7e215064ULL)
#define AUX_MU_BAUD_REG bus_to_reg32(0x7e215068ULL)

// Bit fields for the AUX_MU_CNTL_REG register.
#define AUX_MU_CNTL_RX_ENABLE BIT_U32(0)
#define AUX_MU_CNTL_TX_ENABLE BIT_U32(1)

// Bit fields for the AUX_MU_LSR_REG register.
#define AUX_MU_LSR_DATA_READY BIT_U32(0)
#define AUX_MU_LSR_RX_OVERRUN BIT_U32(1)
#define AUX_MU_LSR_TX_EMPTY   BIT_U32(5)
#define AUX_MU_LSR_TX_IDLE    BIT_U32(6)

// Registers for SPI1.
#define AUX_SPI1_CNTL0_REG bus_to_reg32(0x7e215080ULL)
#define AUX_SPI1_CNTL1_REG bus_to_reg32(0x7e215084ULL)
#define AUX_SPI1_STAT_REG  bus_to_reg32(0x7e215088ULL)
#define AUX_SPI1_IO_REG    bus_to_reg32(0x7e215090ULL)
#define AUX_SPI1_PEEK_REG  bus_to_reg32(0x7e215094ULL)

// Registers for SPI2.
#define AUX_SPI2_CNTL0_REG bus_to_reg32(0x7e2150c0ULL)
#define AUX_SPI2_CNTL1_REG bus_to_reg32(0x7e2150c4ULL)
#define AUX_SPI2_STAT_REG  bus_to_reg32(0x7e2150c8ULL)
#define AUX_SPI2_IO_REG    bus_to_reg32(0x7e2150d0ULL)
#define AUX_SPI2_PEEK_REG  bus_to_reg32(0x7e2150d4ULL)

// GPIO registers
#define GPFSEL0   bus_to_reg32(0x7e200000ULL)
#define GPFSEL1   bus_to_reg32(0x7e200004ULL)
#define GPFSEL2   bus_to_reg32(0x7e200008ULL)
#define GPFSEL3   bus_to_reg32(0x7e20000cULL)
#define GPFSEL4   bus_to_reg32(0x7e200010ULL)
#define GPFSEL5   bus_to_reg32(0x7e200014ULL)
// (reserved)                  0x7e200018
#define GPSET0    bus_to_reg32(0x7e20001cULL)
#define GPSET1    bus_to_reg32(0x7e200020ULL)
// (reserved)                  0x7e200024
#define GPCLR0    bus_to_reg32(0x7e200028ULL)
#define GPCLR1    bus_to_reg32(0x7e20002cULL)
// (reserved)                  0x7e200030
#define GPLEV0    bus_to_reg32(0x7e200034ULL)
#define GPLEV1    bus_to_reg32(0x7e200038ULL)
// (reserved)                  0x7e20003c
#define GPEDS0    bus_to_reg32(0x7e200040ULL)
#define GPEDS1    bus_to_reg32(0x7e200044ULL)
// (reserved)                  0x7e200048
#define GPREN0    bus_to_reg32(0x7e20004cULL)
#define GPREN1    bus_to_reg32(0x7e200050ULL)
// (reserved)                  0x7e200054
#define GPFEN0    bus_to_reg32(0x7e200058ULL)
#define GPFEN1    bus_to_reg32(0x7e20005cULL)
// (reserved)                  0x7e200060
#define GPHEN0    bus_to_reg32(0x7e200064ULL)
#define GPHEN1    bus_to_reg32(0x7e200068ULL)
// (reserved)                  0x7e20006c
#define GPLEN0    bus_to_reg32(0x7e200070ULL)
#define GPLEN1    bus_to_reg32(0x7e200074ULL)
// (reserved)                  0x7e200078
#define GPAREN0   bus_to_reg32(0x7e20007cULL)
#define GPAREN1   bus_to_reg32(0x7e200080ULL)
// (reserved)                  0x7e200084
#define GPAFEN0   bus_to_reg32(0x7e200088ULL)
#define GPAFEN1   bus_to_reg32(0x7e20008cULL)
// (reserved)                  0x7e200090
#define GPPUD     bus_to_reg32(0x7e200094ULL)
#define GPPUDCLK0 bus_to_reg32(0x7e200098ULL)
#define GPPUDCLK1 bus_to_reg32(0x7e20009cULL)
// (reserved)                  0x7e2000a0
// (test)                      0x7e2000b0

// Function select configuration (on 3 bits).
#define GPFSEL_IN   0
#define GPFSEL_OUT  1
#define GPFSEL_ALT0 4
#define GPFSEL_ALT1 5
#define GPFSEL_ALT2 6
#define GPFSEL_ALT3 7
#define GPFSEL_ALT4 3
#define GPFSEL_ALT5 2

// Configuration for GPPUD (on 2 bits).
#define GPPUD_OFF       0
#define GPPUD_PULL_DOWN 1
#define GPPUD_PULL_UP   2
/* clang-format on */
