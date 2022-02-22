#ifndef DEVICE_H
#define DEVICE_H

/* see vmm_pgtable.c for memory layout */

/* the stack space vaddr starts at 16GiB
 * and lasts for 2 MiB
 *
 * this space maps to the phsycial space
 * that .stack lives in
 */
#define STACK_BASE_ADDR (16 * GiB)

void init_device(void* fdt);

extern u64 __cache_line_size;  /* cache line of minimum size */
extern u64 __cache_line_shift;  /* cache line of minimum size */
#define CACHE_LINE_SIZE __cache_line_size
#define CACHE_LINE_SHIFT __cache_line_shift
#define PAGE_SIZE 4096L
#define PAGE_SHIFT 12
#define PMD_SIZE (PAGE_SIZE * 512)
#define PMD_SHIFT 21
#define PUD_SIZE (PMD_SIZE * 512)
#define PUD_SHIFT 30
#define PGD_SIZE (PUD_SIZE * 512)
#define PGD_SHIFT 39

extern u8 __dc_zero_allow;  /* DC ZVA allow ? */
extern u64 __dc_zero_block_width;  /* size of DC ZVA block */

#define DCZVA_ALLOW __dc_zero_allow
#define DCZVA_BLOCK_WIDTH __dc_zero_block_width

extern u64 __asid_size;
#define ASID_SIZE __asid_size

/** PSCI Mandatory Functions
 * While these are defined in the psci node of the dtb
 * they are architecturally hard-coded to the following values in PSCIv0.2 */
#define PSCI_SYSTEM_OFF 0x84000008UL
#define PSCI_CPU_ON 0xC4000003UL  /* SMC64 starts at 0xC4... */
#define PSCI_CPU_OFF 0x84000002UL

#define MAX_CPUS 4

/** number of physical hardware threads */
u64 NO_CPUS;

/* top and bottom of physical memory */
u64 TOP_OF_MEM;

/* bottom of RAM, note: this does not include any IO or bootloader sections
 */
u64 BOT_OF_MEM;

/* total amount of memory */
u64 TOTAL_MEM;

/* total allocated heap space */
u64 TOTAL_HEAP;

/* total allocated space for pagetables */
u64 TOTAL_TABLE_SPACE;

/* bottom and top of heap space
 * used for non-test data
 */
u64 BOT_OF_HEAP;
u64 TOP_OF_HEAP;

/* bottom and top of pagetable space
 * used for test pagetables
 */
u64 BOT_OF_PTABLES;
u64 TOP_OF_PTABLES;

/* bot and top of memory mapped I/O section
 *
 * in practice this is just the physical page the memory-mapped serial
 * controls are located in
 */
u64 BOT_OF_IO;
u64 TOP_OF_IO;

/** per-thread stack size
 */
#define STACK_SIZE (2 * MiB)

/**
 * Top and Bottom of thread stack PA space
 */
u64 TOP_OF_STACK_PA;
u64 BOT_OF_STACK_PA;

/** each thread gets 2 2M regions for its stack
 * the first for EL0 and the second for EL1
 *
 * for CPU#3 with STACK bottom of 0x1000_0000
 *  EL0 is from [0x1200_0000 -> 0x1000_0000]
 *  EL1 is from [0x1400_0000 -> 0x1200_0000]
 */
#define STACK_PYS_THREAD_TOP_ELx(cpu,el) (BOT_OF_STACK_PA + (1+cpu)*2*STACK_SIZE - STACK_SIZE*(1-el))
#define STACK_PYS_THREAD_BOT_ELx(cpu,el) (STACK_PYS_THREAD_TOP_ELx(cpu,el) - STACK_SIZE)

#define STACK_PYS_THREAD_TOP_EL0(cpu) STACK_PYS_THREAD_TOP_ELx(cpu, 0)
#define STACK_PYS_THREAD_TOP_EL1(cpu) STACK_PYS_THREAD_TOP_ELx(cpu, 1)

#define STACK_PYS_THREAD_BOT_EL0(cpu) STACK_PYS_THREAD_BOT_ELx(cpu, 0)
#define STACK_PYS_THREAD_BOT_EL1(cpu) STACK_PYS_THREAD_BOT_ELx(cpu, 1)

/**
 * Sections from linker
 */
u64 BOT_OF_DATA;
u64 TOP_OF_DATA;
u64 TOP_OF_TEXT;
u64 BOT_OF_TEXT;
u64 TOP_OF_DATA;

/** the harness maps all of memory starting at 64G
 */
u64* HARNESS_MMAP;

/** the tests themselves have a chunk of virtual address space allocated at
 * 128G
 *
 * which may be fragmented over physical address space TOP_OF_HEAP -> TOP_OF_MEM
 */
u64* TESTDATA_MMAP;

/* top and bottom of physical regions for the testdata
 */
u64 BOT_OF_TESTDATA;
u64 TOP_OF_TESTDATA;

/**
 * physical adddress the vector tables are located at
 *
 * this is the start of a NO_CPUS*VTABLE_SIZE region
 * where each VTABLE_SIZE (aka PAGE_SIZE) space is
 * one thread's exception vector table
 */
u64 vector_base_pa;

/**
 * from dtb
 */
extern char* fdt_load_addr;

/**
 * see https://github.com/devicetree-org/devicetree-specification/blob/master/source/flattened-format.rst
 */
typedef struct  {
    u32 fdt_magic;
    u32 fdt_totalsize;
    u32 fdt_off_dt_struct;
    u32 fdt_off_dt_strings;
    u32 fdt_off_mem_rsvmap;
    u32 fdt_version;
    u32 fdt_last_comp_version;
    u32 fdt_boot_cpuid_phys;
    u32 fdt_size_dt_strings;
    u32 fdt_size_dt_struct;
} fdt_header;

typedef struct {
    u32 token;
    u32 nameoff;
} fdt_structure_piece_with_name;

typedef struct {
    char* current;
    u32 token;
    char* next;
} fdt_structure_piece;

typedef struct {
    u32 token;
    u32 len;
    u32 nameoff;
    char data[0];
} fdt_structure_property_header;

typedef struct {
    u32 token;
    char name[0];
} fdt_structure_begin_node_header;

fdt_structure_piece fdt_find_node_with_prop_with_index(char* fdt, char* index, char* prop_name, char* expected_value);
fdt_structure_begin_node_header* fdt_find_node(char* fdt, char* node_name);
fdt_structure_begin_node_header* fdt_read_node(char* fdt, fdt_structure_begin_node_header* node, char* node_name);
fdt_structure_property_header* fdt_read_prop(char* fdt, fdt_structure_begin_node_header* node, char* prop_name);
fdt_structure_property_header* fdt_find_prop(char* fdt, char* node_name, char* prop_name);

char* dtb_bootargs(void* fdt);

typedef struct {
  u64 base;
  u64 size;
  u64 top;  /* base+size */
} dtb_mem_t;

dtb_mem_t dtb_read_memory(void* fdt);
dtb_mem_t dtb_read_ioregion(void* fdt);

#endif /* DEVICE_H */
