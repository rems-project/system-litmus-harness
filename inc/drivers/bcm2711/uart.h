#ifndef BCM2711_H
#define BCM2711_H

/* RPi4 header */

/* for mini uart */
#define BCM2711_AUX_BASE 0x7E215000

/* for GPIO and PL011 UART */
#define BCM2711_GPIO_BASE 0x7E200000

/** BCM2711 requires setting up the UART */
void init_uart(void);

#endif /* BCM2711_H */
