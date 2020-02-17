#include "lib.h"

extern void MP_pos(void);
extern void MP_dmb_svc(void);
extern void MP_dmb_eret(void);

int main(void) {
    NUMBER_OF_RUNS = 100000UL;
    ENABLE_PGTABLE = 0;
    MP_pos();
    MP_dmb_svc();
    MP_dmb_eret();
    return 0;
}
