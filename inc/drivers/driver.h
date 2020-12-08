#ifndef DRIVER_H
#define DRIVER_H

/** call to initialize SoC-specific data
 * to some standard configuration
 */
void init_driver(void);

/** write 1 byte to stdout */
void write_stdout(uint8_t);

#endif /* DRIVER_H */
