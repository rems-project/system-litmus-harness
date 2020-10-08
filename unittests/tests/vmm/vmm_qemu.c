#include "lib.h"

#include "testlib.h"

#include "drivers/qemu/qemu.h"

UNIT_TEST(test_vmm_qemu_uart)
void test_vmm_qemu_uart(void) {
  uint64_t* pgtable = vmm_alloc_new_4k_pgtable();
  ASSERT((uint64_t)vmm_pa(pgtable, UART0_BASE) == UART0_BASE);
}

UNIT_TEST(test_vmm_qemu_uart_translated)
void test_vmm_qemu_uart_translated(void) {
  uint64_t* pgtable = vmm_alloc_new_4k_pgtable();
  if (ENABLE_PGTABLE) {
    vmm_switch_ttable(pgtable);
  } else {
    vmm_set_id_translation(pgtable);
  }
  writeb('\0', UART0_BASE);
  vmm_set_id_translation(NULL);
  ASSERT(1);  /* assert it *reaches* end of test */
}