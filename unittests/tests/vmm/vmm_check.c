#include <stdint.h>

#include "lib.h"
#include "testlib.h"

/** hand-written translation function for 4k pages
 */
uint64_t translate4k(uint64_t* root, uint64_t vaddr) {
  uint64_t level, desc;
  uint64_t offs, bot, top;

  level = 0;
  while (1) {
    uint64_t valid, table;
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
      root = (uint64_t*)(desc & ~((1UL << 12) - 1));
      level++;
    } else {
      puts("! translate4k: unknown?\n");
      abort();
    }
  }

  uint64_t tw = (1UL << (48 - bot)) - 1;
  uint64_t pa = (desc & (tw << bot)) | BIT_SLICE(vaddr, bot, 0);
  return pa;
}

UNIT_TEST(test_translate_correct)
void test_translate_correct(void) {
  uint64_t* pgtable = vmm_alloc_new_idmap_4k();
  uint64_t va = 0x47FEE000;
  ASSERT(translate4k(pgtable, va) == (uint64_t)vmm_pa(pgtable, va));
}

UNIT_TEST(test_translate_identity)
void test_translate_identity(void) {
  uint64_t* pgtable = vmm_alloc_new_idmap_4k();
  uint64_t va = 0x47FEE000;
  ASSERT(va == (uint64_t)vmm_pa(pgtable, va));
}