#include "lib.h"

static const char* indents[4] = {
  "",           /* L0 */
  " +",         /* L1 */
  " + +",       /* L2 */
  " + + +",     /* L3 */
};

void __vmm_dump_node(uint64_t* parent_table, int level, int index, uint64_t* entry, desc_t desc, int is_leaf, va_range range, void* data) {
  if (! is_leaf)
    debug("%s(%p -> %p) [%d] %p\n", indents[level], range.start, range.end, level, desc.table_addr);
  else
    debug("%s(%p -> %p) [%d:%d]\n", indents[level], range.start, range.end, level, index);
}

void __vmm_count_tables(uint64_t* parent_table, int level, int index, uint64_t* entry, desc_t desc, int is_leaf, va_range range, void* data) {
  uint64_t* ctr = data;
  if (! is_leaf)
    *ctr += 1;
}

void vmm_dump_table(uint64_t* ptable) {
  vmm_walk_table(ptable, __vmm_dump_node, NULL);
}

uint64_t vmm_count_subtables(uint64_t* ptable) {
  uint64_t count = 0;
  vmm_walk_table(ptable, __vmm_count_tables, &count);
  return count;
}