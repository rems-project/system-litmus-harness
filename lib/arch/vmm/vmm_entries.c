
#include "lib.h"

/* stage 1 attrs */
attrs_t read_attrs(u64 desc) {
  attrs_t attr = { 0 };
  /* upper attrs */
  attr.XN = BIT(desc, 54);
  attr.PXN = BIT(desc, 53);
  /* lower attrs */
  attr.nT = BIT(desc, 16);
  attr.nG = BIT(desc, 11);
  attr.AF = BIT(desc, 10);
  attr.SH = BIT_SLICE(desc, 9, 8);
  attr.AP = BIT_SLICE(desc, 7, 6);
  attr.NS = BIT(desc, 5);
  attr.attr = BIT_SLICE(desc, 4, 2);
  return attr;
}

desc_t read_desc(u64 entry, int level) {
  desc_t desc;

  desc.level = level;

  if (BIT(entry, 0) == 0) {
    desc.type = Invalid;
    return desc;
  }

  switch (level) {
  case 0:
    /* 0th level table can only be a Table */
    desc.table_addr = BIT_SLICE(entry, 47, 12) << 12;
    desc.type = Table;
    break;

  case 1:
  case 2:
    /* 1-2 could be table or block */
    switch (BIT(entry, 1)) {
    case 0:
      desc.type = Block;
      desc.oa = BIT_SLICE(entry, 47, OFFSBOT(level)) << OFFSBOT(level);
      break;
    case 1:
      desc.table_addr = BIT_SLICE(entry, 47, 12) << 12;
      desc.type = Table;
      break;
    }
    break;

  case 3:
    /* level3 entry must be a block */
    desc.type = Block;
    desc.oa = BIT_SLICE(entry, 47, 12) << 12;
    break;
  }

  desc.attrs = read_attrs(entry);
  return desc;
}

u64 write_attrs(attrs_t* attrs) {
  /* clang-format off */
  u64 enc_attrs = \
    0                       |
    ((u64)attrs->XN << 54)  |
    ((u64)attrs->PXN << 53) |
    (attrs->nT << 16)       |
    (attrs->nG << 11)       |
    (attrs->AF << 10)       |
    (attrs->SH << 8)        |
    (attrs->AP << 6)        |
    (attrs->NS << 5)        |
    (attrs->attr << 2)      ;
  /* clang-format on */
  return enc_attrs;
}

u64 write_desc(desc_t desc) {
  u64 out = 0;
  switch (desc.type) {
  case Invalid:
    return 0;
  case Table:
    out = desc.table_addr;
    out |= 2;
    break;
  case Block:
    out = desc.oa;

    if (desc.level == 3) {
      out |= 2;
    }
    break;
  }

  out |= write_attrs(&desc.attrs);
  out |= 1; /* valid */
  return out;
}

void show_attrs(attrs_t* attrs) {
  printf(
    "{XN:%d, PXN:%d, nT:%d, OA[51:48]:%d, nG:%d, AF:%d, SH:%d, AP:%d, NS:%d, attr:%d}",
    attrs->XN,
    attrs->PXN,
    attrs->nT,
    attrs->OA,
    attrs->nG,
    attrs->AF,
    attrs->SH,
    attrs->AP,
    attrs->NS,
    attrs->attr
  );
}

void show_desc(desc_t desc) {
  switch (desc.type) {
  case Invalid:
    printf("<Invalid>");
    return;

  case Block:
    printf("<Block oa=%p", desc.oa);
    break;

  case Table:
    printf("<Table table_addr=%p", desc.table_addr);
    break;
  }

  printf(",level=%d", desc.level);
  printf(",attrs=");
  show_attrs(&desc.attrs);
  printf(">");
}