#ifndef DEVICE_H
#define DEVICE_H
#include <stdint.h>

void init_device(void);

/** number of physical hardware threads */
uint64_t NO_CPUS;

/* top of physical memory */
uint64_t TOP_OF_MEM;

/* total allocated heap space */
uint64_t TOTAL_HEAP;

/* bottom of heap space */
uint64_t BOT_OF_HEAP;

/**
 * Top and Bottom of thread stacks
 */
uint64_t TOP_OF_STACK;
uint64_t BOT_OF_STACK;

/**
 * Sections from linker
 */
extern unsigned long text_end; /* end of .text section (see bin.lds) */
extern unsigned long data_end; /* end of .bss and .rodata section */
extern unsigned long stacktop; /* end of .bss and .rodata section */

uint64_t TOP_OF_TEXT;
uint64_t TOP_OF_DATA;

/**
 * from dtb
 */
extern char* fdt;

/**
 * see https://github.com/devicetree-org/devicetree-specification/blob/master/source/flattened-format.rst
 */
typedef struct  {
    uint32_t fdt_magic;
    uint32_t fdt_totalsize;
    uint32_t fdt_off_dt_struct;
    uint32_t fdt_off_dt_strings;
    uint32_t fdt_off_mem_rsvmap;
    uint32_t fdt_version;
    uint32_t fdt_last_comp_version;
    uint32_t fdt_boot_cpuid_phys;
    uint32_t fdt_size_dt_strings;
    uint32_t fdt_size_dt_struct;
} fdt_header;

typedef struct {
    uint32_t token;
    uint32_t nameoff;
} fdt_structure_piece_with_name;

typedef struct {
    uint32_t token;
    char* next;
} fdt_structure_piece;

typedef struct {
    uint32_t token;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
} fdt_structure_property_header;

typedef struct {
    uint32_t token;
    char name[0];
} fdt_structure_begin_node_header;

fdt_structure_property_header* fdt_read_prop(char* fdt, char* node_name, char* prop_name);
char* dtb_bootargs(void* fdt);

#endif /* DEVICE_H */
