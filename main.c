#include "lib.h"

extern void MyMP_pos(void);
extern void MyMP_dmb_svc(void);

int main(void) {
    NUMBER_OF_RUNS = 100000UL;
    MyMP_pos();
    MyMP_dmb_svc();
    return 0;
}