#ifndef BCM2837_ADDR_H
#define BCM2837_ADDR_H

/* BCM2837 maps the periphrals
 * between 0x3F000000 -> 0x3FFFFFFF
 * with the System MMU setup to map
 * 0x7E000000 -> 0x3F000000
 *
 * so addresses marked as 0x7Exxxxx
 * should be accessed via 0x3Fxxxxx instead.
 */
#define __PERIPH_OFFSET (0x7E000000UL - 0x3F000000UL)
#define PERIPHERAL(addr) ((addr)-__PERIPH_OFFSET)

#endif /* BCM2837_ADDR_H */
