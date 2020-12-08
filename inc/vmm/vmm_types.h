#ifndef VMM_TYPES_H
#define VMM_TYPES_H
#include <stdint.h>

enum DescType {
  Table,
  Block,
  Invalid
};

typedef struct {
  /* upper attrs */
  unsigned int XN:1;
  unsigned int PXN:1;

  /* lower attrs */
  unsigned int nT:1;
  unsigned int OA:4;
  unsigned int nG:1;
  unsigned int AF:1;
  unsigned int SH:2;
  unsigned int AP:2;
  unsigned int NS:1;
  unsigned int attr:3;
} attrs_t;

typedef struct {
  union {
    uint64_t oa;
    uint64_t table_addr;
  };

  /* src/dest of this desc (if applicable) */
  union {
    uint64_t* src;
    uint64_t* dest;
  };

  /* parent srcs (if applicable) */
  uint64_t* parents[4];

  enum DescType type;
  int level;
  attrs_t attrs;
} desc_t;


attrs_t read_attrs(uint64_t desc);
desc_t read_desc(uint64_t desc, int level);

uint64_t write_attrs(attrs_t attrs);
uint64_t write_desc(desc_t desc);


/* for debug */
void show_attrs(attrs_t attrs);
void show_desc(desc_t desc);
/* end for debug */

#endif /* VMM_TYPES_H */
