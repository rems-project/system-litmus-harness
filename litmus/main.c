#include "lib.h"

extern void MP_pos(void);
extern void MP_dmb_svc(void);
extern void MP_dmb_eret(void);
extern void CoWT(void);
extern void CoWT_dsb(void);
extern void CoWTinv(void);
extern void CoWTinv_dsb(void);

uint64_t NUMBER_OF_RUNS = 1000UL;
uint8_t ENABLE_PGTABLE = 1;

int main(void) {
  /** warning:
   * ensure ENABLE_PGTABLE is set to 0 for exceptions tests
   */
  MP_pos();

  if (ENABLE_PGTABLE) {
    CoWT();
    CoWT_dsb();
    CoWTinv();
    CoWTinv_dsb();
  }

  if (! ENABLE_PGTABLE) {
    MP_dmb_svc();
    MP_dmb_eret();
  }
  return 0;
}
