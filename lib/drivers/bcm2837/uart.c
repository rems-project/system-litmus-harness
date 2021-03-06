#include "lib.h"

#include "drivers/bcm2837/uart.h"
#include "drivers/bcm2837/gpio.h"

static void __delay(int count) {
  asm (
    "mov x0, %[c]\n"
    "cbz x0, 1f\n"
    "0:\n"
    "sub x0, x0, #1\n"
    "cbnz x0, 0b\n"
    "1:\n"
  :
  : [c] "r" (count)
  : "memory"
  );
}

void init_uart(void) {
  /* set GPIO pins 14/15 to be the mini UART */
  uint32_t sel = readw(GPFSEL1);
  sel &= ~(BITMASK(3) << GPFSEL1_FSEL_15);
  sel &= ~(BITMASK(3) << GPFSEL1_FSEL_15);

  writew(
    sel
      | (GPFSEL_ALT5 << GPFSEL1_FSEL_14)
      | (GPFSEL_ALT5 << GPFSEL1_FSEL_15),
    GPFSEL1
  );

  /* according to page 101 of the datasheet for BCM2837
   * we are recommended to add a sleep of 150 cycles
   * to ensure the control signal propagates
   */
  writew(GPPUD_Disable, GPPUD);
  __delay(150);
  /* apply to pins 14 and 15, aka our mini UART */
  writew((1 << 14) | (1 << 15), GPPUDCLK0);
  __delay(150);
  writew(0, GPPUDCLK0);

  /* enable the mini UART
   * and configure it */
  writew(AUXENB_MiniUART_Enable, AUXENB);
  writew(0, AUX_MU_CNTL_REG);
  writew(AUX_MU_LCR_DATA_8BIT, AUX_MU_LCR_REG);
  writew(AUX_MU_BAUD_115200, AUX_MU_BAUD_REG);
  writew(3, AUX_MU_CNTL_REG);
  writew(0, PERIPHERAL(0x7E215050UL));
}

void write_stdout(uint8_t c) {
  /* block until the transmit pin is empty
   * and able to accept another byte.
   */
  while(1) {
    if (readw(AUX_MU_LSR_REG) & AUX_MU_LSR_TX_EMPTY)
      break;
  }

  /* write to the mini UART TX pin
   * which will be GPIO pin 14 if init_uart has been run
   * via the memory mapped AUX_MU_IO_REG register.
   */
  writew(c, AUX_MU_IO_REG);
}