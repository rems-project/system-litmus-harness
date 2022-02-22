
#include "lib.h"
#include "testlib.h"

#include "drivers/qemu/qemu.h"

/** hand-written translation function for 4k pages
 */
u64 translate4k(u64* root, u64 vaddr) {
  u64 level, desc;
  u64 offs, bot, top;

  level = 0;
  while (1) {
    u64 valid, table;
    switch (level) {
      case 0:
        top = 48;
        bot = 39;
        break;
      case 1:
        top = 38;
        bot = 30;
        break;
      case 2:
        top = 29;
        bot = 21;
        break;
      case 3:
        top = 20;
        bot = 12;
        break;
    }

    offs = BIT_SLICE(vaddr, top, bot);
    desc = root[offs];
    valid = (desc & 0x1) == 0x1;
    table = (desc & 0x2) == 0x2;

    if (!valid || (level == 3 && !table)) {
      return 0;
    }

    if ((level < 3 && !table) || (level == 3 && table)) {
      break;
    }

    if (level < 3 && table) {
      root = (u64*)(desc & ~((1UL << 12) - 1));
      level++;
    } else {
      puts("! translate4k: unknown?\n");
      abort();
    }
  }

  u64 tw = (1UL << (48 - bot)) - 1;
  u64 pa = (desc & (tw << bot)) | BIT_SLICE(vaddr, bot, 0);
  return pa;
}

UNIT_TEST(test_translate_correct)
void test_translate_correct(void) {
  u64* pgtable = vmm_alloc_new_4k_pgtable();
  ASSERT(translate4k(pgtable, 0x47FEE000UL) == (u64)vmm_pa(pgtable, 0x47FEE000UL), "0x47FEE000");
  ASSERT(translate4k(pgtable, UART0_BASE) == (u64)vmm_pa(pgtable, UART0_BASE), "UART0_BASE");
  ASSERT(translate4k(pgtable, 0x40081234UL) == (u64)vmm_pa(pgtable, 0x40081234UL), "0x40081234");
}

UNIT_TEST(test_translate_identity)
void test_translate_identity(void) {
  u64* pgtable = vmm_alloc_new_4k_pgtable();
  ASSERT(0x47FEE000UL == (u64)vmm_pa(pgtable, 0x47FEE000UL), "0x47FEE000");
  ASSERT(UART0_BASE == (u64)vmm_pa(pgtable, UART0_BASE), "UART0_BASE");
  ASSERT(0x40081234UL == (u64)vmm_pa(pgtable, 0x40081234UL), "0x40081234");
}