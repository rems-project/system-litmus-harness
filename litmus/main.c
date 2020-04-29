#include "lib.h"
#include "frontend.h"

extern litmus_test_t
    SB_pos,
    SB_dmbs,
    MP_dmbs,
    MP_pos,
    WRC_pos,
    WRC_addrs,
    WRCat_ctrl_dsb,
    CoWT1_dsbtlbidsb,
    CoWT1_dsbtlbidsbisb,
    CoWT_dsbsvctlbidsb,
    MPRT_svcdsbtlbiisdsb_dsbisb,
    MPRT1_dsbtlbiisdsb_dsbisb,
    CoWTinv_dsb,
    CoWT_dsb,
    CoWT,
    CoWTinv,
    CoWTinv_dsbisb,
    CoWT_dsbisb,
    BBM1_dsbtlbiisdsbdsb_dsbisb,
    BBM1_dsbtlbiisdsbdsb_dsb,
    BBM1_dsbtlbiisdsbdsbisb_dsbisb,
    BBM1_dsbtlbiisdsbdsbisb_dsb,
    MPRT_svcdsbtlbidsb_dsbisb,
    MPRT1_dsbtlbidsb_dsbisb,
    MPRT1_dsbtlbialldsb_dsbisb,
    MPRT1_dsbtlbiisdsbtlbiisdsb_dsbisb,
    MPRT1_dcdsbtlbialldsb_dsbisb,
    MPRT1_dsbtlbiallisdsb_dsbisb,
    MPRT1_dsbtlbiisdsbisb_dsbisb,
    MP_dmb_eret,
    MP_dmb_svc,
    MPRTinv_dmb_addr,
    WRCtrr_addr_dmb,
    WRCtrrinv_addrs,
    WRCtrt_addr_dmb,
    WRCtrt_dmbs,
    WRCtrt_dsbisbs,
    WRCtrtinv_dsbisbs,
    WRCtrtinv_po_dmb,
    WRCtrtinv_po_addr,
    WRCtrt1_dsbtlbiisdsb_dmb,
    CoTR,
    CoTRinv,
    CoTRinv_dsbisb,
    CoTR_addr,
    CoTR_dmb,
    CoTR_dsb,
    CoTR_dsbisb,
    CoTR1_dsbtlbidsbisb,
    CoTR1_dsbdcdsbtlbidsbisb,
    CoTR1tlbi_dsbdsbisb,
    CoWinvT,
    CoWinvT1_dsbtlbidsb,
    ISA2trr_dmb_po_dmb;


const litmus_test_group data_group = {
  .name="@data",
  .tests = (const litmus_test_t*[]){
    &MP_pos,
    &MP_dmbs,
    &SB_pos,
    &SB_dmbs,
    &WRC_pos,
    &WRC_addrs,
    NULL,
  }
};

const litmus_test_group exc_group = {
  .name = "@exc",
  .tests = (const litmus_test_t*[]){
    &MP_dmb_svc,
    &MP_dmb_eret,
    NULL,
  }
};

const litmus_test_group pgtable_group = {
  .name = "@pgtable",
  .tests = (const litmus_test_t*[]){
    &BBM1_dsbtlbiisdsbdsb_dsbisb,
    &BBM1_dsbtlbiisdsbdsb_dsb,
    &BBM1_dsbtlbiisdsbdsbisb_dsbisb,
    &BBM1_dsbtlbiisdsbdsbisb_dsb,
    &WRCat_ctrl_dsb,
    &CoWT1_dsbtlbidsb,
    &CoWT1_dsbtlbidsbisb,
    &CoWT_dsbsvctlbidsb,
    &MPRT_svcdsbtlbiisdsb_dsbisb,
    &MPRT1_dsbtlbiisdsb_dsbisb,
    &MPRT1_dsbtlbialldsb_dsbisb,
    &MPRT1_dsbtlbiisdsbtlbiisdsb_dsbisb,
    &MPRT1_dsbtlbiallisdsb_dsbisb,
    &MPRT1_dsbtlbiisdsbisb_dsbisb,
    &CoWTinv_dsb,
    &CoWTinv_dsbisb,
    &CoWT_dsb,
    &CoWT,
    &CoWTinv,
    &CoWinvT,
    &CoWinvT1_dsbtlbidsb,
    &CoWT_dsbisb,
    &CoTR,
    &CoTRinv,
    &CoTRinv_dsbisb,
    &CoTR_addr,
    &CoTR_dmb,
    &CoTR_dsb,
    &CoTR_dsbisb,
    &CoTR1_dsbtlbidsbisb,
    &CoTR1_dsbdcdsbtlbidsbisb,
    &CoTR1tlbi_dsbdsbisb,
    &MPRT_svcdsbtlbidsb_dsbisb,
    &MPRT1_dsbtlbidsb_dsbisb,
    &MPRT1_dcdsbtlbialldsb_dsbisb,
    &MP_dmb_eret,
    &MP_dmb_svc,
    &MPRTinv_dmb_addr,
    &WRCtrr_addr_dmb,
    &WRCtrrinv_addrs,
    &WRCtrt_addr_dmb,
    &WRCtrt_dmbs,
    &WRCtrt_dsbisbs,
    &WRCtrtinv_dsbisbs,
    &WRCtrt1_dsbtlbiisdsb_dmb,
    &WRCtrtinv_po_dmb,
    &WRCtrtinv_po_addr,
    &ISA2trr_dmb_po_dmb,
    NULL,
  }
};

const litmus_test_group all = {
  .name="@all",
  .groups = (const litmus_test_group*[]){
    &data_group,
    &exc_group,
    &pgtable_group,
    NULL,
  }
};

litmus_test_t check1, check2, check3, MPtimes_pos;
const litmus_test_group timing_group = {
  .name="@timing",
  .tests = (const litmus_test_t*[]){
    &MPtimes_pos,
    NULL,
  }
};

const litmus_test_group checks = {
  .name="@checks",
  .tests = (const litmus_test_t*[]){
    &check1,
    &check2,
    &check3,
    NULL,
  }
};

const litmus_test_group _real_all = {
  .name=NULL,
  .groups = (const litmus_test_group*[]){
    &all,
    &checks,
    &timing_group,
    NULL,
  },
};

/** if 1 then don't run just check */
uint8_t dry_run = 0;


int main(int argc, char** argv) {
  if (ONLY_SHOW_MATCHES) {
    for (int i = 0; i < collected_tests_count; i++) {
      /* @_real_all is a phony group and contains things that should not be run when using @all
      * but can be specifcally selected */
      re_t* re = re_compile(collected_tests[i]);
      show_matches_only(&all, re);
      re_free(re);
    }

    return 0;
  }

  if (collected_tests_count == 0) {
    re_t* re = re_compile("@all");
    match_and_run(&all, re);  /* default to @all */
    re_free(re);
  } else {
    /* first do a dry run, without actually running the functions
      * just to validate the arguments */
    for (uint8_t r = 0; r <= 1; r++) {
      dry_run = 1 - r;

      for (int i = 0; i < collected_tests_count; i++) {
        /* @_real_all is a phony group and contains things that should not be run when using @all
        * but can be specifcally selected */
        re_t* re = re_compile(collected_tests[i]);
        match_and_run(&_real_all, re);
        re_free(re);
      }
    }
  }
  return 0;
}
