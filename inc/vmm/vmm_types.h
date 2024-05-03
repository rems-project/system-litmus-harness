#ifndef VMM_TYPES_H
#define VMM_TYPES_H

enum DescType { Table, Block, Invalid };

typedef struct
{
  /* upper attrs */
  unsigned int XN  : 1;
  unsigned int PXN : 1;

  /* lower attrs */
  unsigned int nT   : 1;
  unsigned int OA   : 4;
  unsigned int nG   : 1;
  unsigned int AF   : 1;
  unsigned int SH   : 2;
  unsigned int AP   : 2;
  unsigned int NS   : 1;
  unsigned int attr : 3;
} attrs_t;

typedef struct
{
  union {
    u64 oa;
    u64 table_addr;
  };

  /* src/dest of this desc (if applicable) */
  union {
    u64* src;
    u64* dest;
  };

  /* parent srcs (if applicable) */
  u64* parents[4];

  enum DescType type;
  int level;
  attrs_t attrs;
} desc_t;

attrs_t read_attrs(u64 desc);
desc_t read_desc(u64 desc, int level);

u64 write_attrs(attrs_t* attrs);
u64 write_desc(desc_t desc);

/* for debug */
void show_attrs(attrs_t* attrs);
void show_desc(desc_t desc);
/* end for debug */

#endif /* VMM_TYPES_H */
