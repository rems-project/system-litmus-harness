#include "lib.h"

extern void MP_pos(void);
extern void MP_dmbs(void);
extern void MP_dmb_svc(void);
extern void MP_dmb_eret(void);
extern void CoWT(void);
extern void CoWT_dsb(void);
extern void CoWTinv(void);
extern void CoWTinv_dsb(void);

int main(void) {
  MP_pos();
  return 0;
  MP_dmbs();

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
