#include "lib.h"

void init_device(void) {
    /* some of these are hard-coded for now but should really be read from the dtb and lds */
    TOP_OF_MEM = 0x48000000UL;
    TOP_OF_STACK = (uint64_t)&stacktop;
    BOT_OF_STACK = (uint64_t)&data_end;
    TOTAL_HEAP = TOP_OF_MEM - TOP_OF_STACK;
    BOT_OF_HEAP = TOP_OF_MEM - TOTAL_HEAP;
    TOP_OF_TEXT = (uint64_t)&text_end;
    TOP_OF_DATA = BOT_OF_STACK;
}