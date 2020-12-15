#ifndef BCM2837_GPIO_H
#define BCM2837_GPIO_H

#include "drivers/bcm2837/addr.h"

/** if we wish to use the mini UART then
 * it's a good idea to replace GPIO pins 14/15
 * to be the mini UART for consistency
 *
 * to do that, there is a memory-mapped register
 * to select alternate functions for those pins
 */
#define GPFSEL1 PERIPHERAL(0x7E200004UL)

#define GPFSEL1_FSEL_14 12
#define GPFSEL1_FSEL_15 15

#define GPFSEL_ALT5 2

/** to disable pin pull up/down we
 * write to the GPPUD memory mapped register
 * which is a control register which we can then use
 * the GPPUDCLK register to apply to a pin
 *
 * we want to do this for pins 14/15 since they are always connected
 * so we shouldn't pull them up or down
 */
#define GPPUD PERIPHERAL(0x7E200094UL)

#define GPPUD_Disable 0

/** for changes to the GPPUD register to be observed
 * we have to toggle a bit in the GPPUDCLK0/1 registers
 *
 * in this case we want to toggle pins 14/15 for mini UART
 * so we only need CLK0
 */
#define GPPUDCLK0 PERIPHERAL(0x7E200098UL)

#endif /* BCM2837_GPIO_H */
