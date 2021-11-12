#ifndef BCM2837_H
#define BCM2837_H

#include "drivers/bcm2837/addr.h"

/* RPi 3B+ header */

/* the mini UART, which we will use for
 * serial output, is defined as an auxilliary device.
 *
 * we enable auxilliary devices by writing to the AUXENB register
 * memory mapped to this address
 */
#define AUXENB PERIPHERAL(0x7E215004UL)
#define AUXENB_MiniUART_Enable 1

/* to write to the mini UART,
 * its I/O Register is memory mapped to this address
 *
 * write to bits 7:0 to transmit
 */
#define AUX_MU_IO_REG PERIPHERAL(0x7E215040UL)

/** the mini UART is either 7 or 8 bit
 * and we can set that by writing to the following memory mapped register
 */
#define AUX_MU_LCR_REG PERIPHERAL(0x7E21504CUL)

#define AUX_MU_LCR_DATA_8BIT 3

/** the baud rate of the Mini UART is controlled
 * by a combination of the system clock freq and the following
 * memory mapped register
 */
#define AUX_MU_BAUD_REG PERIPHERAL(0x7E215068UL)

/* the mini UART baud rate is
 * calculated as follows:
 *
 *  baud = system_clock_freq / (8*(AUX_MU_BAUD_REG + 1))
 *
 * the RPi 3B+ system clock is 400MHz on reset
 * so for the standard 115200 Baud rate
 * we need
 * 115200baud = 400MHz / (8*(r + 1))
 * =>
 * r = ((400 * 10^6) / (8*115200)) - 1
 * =>
 * r = 433.027...
 * aka r = 433
 *
 * for 250MHz same calculation:
 *  250MHz / (8*(r+1))
 * => r = ((250 * 10^6) / (8 * 115200)) - 1
 * => r = 270.267...
 */
#define AUX_MU_BAUD_115200_400MHz 433
#define AUX_MU_BAUD_115200_250MHz 270

/** the mini UART is controlled by this control register
 * which enables/disables transmit/receive
 */
#define AUX_MU_CNTL_REG PERIPHERAL(0x7E215060UL)

#define AUX_MU_CNTL_TX_ENABLE 2
#define AUX_MU_CNTL_RX_ENABLE 1

/** when we do try send on the mini UART
 * we have to check it's not busy first
 * we do that by reading the following memory mapped register
 * and checking the transmitter is idle and empty
 */
#define AUX_MU_LSR_REG PERIPHERAL(0x7E215054UL)

#define AUX_MU_LSR_TX_EMPTY (1 << 5)

/** interrupt enable register */
#define AUX_MU_IER_REG PERIPHERAL(0x7e215044UL)

#define AUX_MU_IER_DISABLE_INTERRUPTS 0

/** interrupt status register */
#define AUX_MU_IIR_REG PERIPHERAL(0x7E215048UL)

#define AUX_MU_IIR_CLEAR_TX_FIFO 2
#define AUX_MU_IIR_CLEAR_RX_FIFO 4

/** modern control register */
#define AUX_MU_MCR_REG PERIPHERAL(0x7E215050UL)

/** BCM2711 requires setting up the UART
 * before it can be used
 */
void init_uart(void);

#endif /* BCM2837_H */
