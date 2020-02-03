#include "lib.h"

extern void MyMP_pos(void);

int main(void) {
    puts("Hello, World!\n");

    NUMBER_OF_RUNS = 10000;
    MyMP_pos();
    return 0;
}