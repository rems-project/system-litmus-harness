#include "lib.h"

#include "drivers/bcm2711/uart.h"

#define AUX_ENABLES ((BCM2711_AUX_BASE)+0x04)
#define AUX_ENABLE_MINIUART 1

#define MINIUART_IO_REG(field) ((BCM2711_AUX_BASE)+0x40+field)
#define MINIUART_IO_REG_TX 1

#define UART_GPIO_MMAP_REG_CR(base) ((base) + 0x30)
#define UART_CR_ENABLE 1
#define UART_CR_TXE (1 << 8)

void init_uart(void) {
  volatile uint32_t* aux_enable = AUX_ENABLES;
  *aux_enable = AUX_ENABLE_MINIUART;
}

void write_stdout(uint8_t c) {
    writeb(c, MINIUART_IO_REG(MINIUART_IO_REG_TX));
}