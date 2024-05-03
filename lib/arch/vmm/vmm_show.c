#include "lib.h"

static const char* indents[4] = {
  "",       /* L0 */
  " +",     /* L1 */
  " + +",   /* L2 */
  " + + +", /* L3 */
};

void __vmm_dump_node(
  u64* parent_table, int level, int index, u64* entry, desc_t desc, int is_leaf, va_range range, void* data
) {
  if (!is_leaf)
    debug("%s(%p -> %p) [%d] %p\n", indents[level], range.start, range.end, level, desc.table_addr);
  else
    debug("%s(%p -> %p) [%d:%d]\n", indents[level], range.start, range.end, level, index);
}

void __vmm_count_tables(
  u64* parent_table, int level, int index, u64* entry, desc_t desc, int is_leaf, va_range range, void* data
) {
  u64* ctr = data;
  if (!is_leaf)
    *ctr += 1;
}

void vmm_dump_table(u64* ptable) {
  vmm_walk_table(ptable, __vmm_dump_node, NULL);
}

u64 vmm_count_subtables(u64* ptable) {
  u64 count = 0;
  vmm_walk_table(ptable, __vmm_count_tables, &count);
  return count;
}