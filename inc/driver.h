#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>

/** to pull some parts out that are driven by other devices that the guest can interact with
 * mostly the interfaces to the emulated QEMU devices sit here */


/** write 1 byte to stdout */
void write_stdout(uint8_t);


#endif /* DRIVER_H */