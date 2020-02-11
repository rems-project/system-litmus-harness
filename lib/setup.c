#include <lib.h>

void setup(void) {
    init_device();
    init_valloc();
    cpu_data_init();
    cpu_data[0].started = 1;
}
