#include "lib.h"

u64 __cache_line_size;  /* cache line of maximum size */
u64 __cache_line_shift;  /* log2(__cache_line_size) */

u8 __dc_zero_allow;  /* DC ZVA allow ? */
u64 __dc_zero_block_width;  /* size of DC ZVA block */

u64 __asid_size;  /* number of ASID bits */

extern char* __ld_begin_text;
extern char* __ld_end_text;
extern char* __ld_begin_reloc;
extern char* __ld_end_reloc;
extern char* __ld_end_sections;
extern char* __ld_begin_data;
extern char* __ld_end_data;

boot_data_t boot_data;

void init_device(void* fdt) {
    NO_CPUS = 4;
    init_driver();

    /* read the memory region from the dtb */
    dtb_mem_t mem_region = dtb_read_memory(fdt);
    TOP_OF_MEM = mem_region.top;
    BOT_OF_MEM = mem_region.base;
    TOTAL_MEM = mem_region.size;

    /* read regions from linker */
    TOP_OF_TEXT = (u64)&__ld_end_text;
    BOT_OF_TEXT = (u64)&__ld_begin_text;
    BOT_OF_DATA = (u64)&__ld_begin_data;
    TOP_OF_DATA = (u64)&__ld_end_data;

    /* compute remaining friendly region names */

    /* we try make a memory layout like so:
     *  ------- TOP
     *    |
     *    |      TEST DATA
     *    |
     *  ~~~~~~~~ 2M
     *    |
     *    |       PAGETABLE ALLOC REGION
     *    |
     *  ------- 4k
     *    |
     *    |       HEAP
     *    |
     *  ------- 4k
     *    |
     *    |       STACK
     *    |
     *  ~~~~~~~~ 2M
     *  ~~~~~~~~
     *    TEXT + DATA
     *  ~~~~~~~
     *     |
     *  ~~~~~~~
     *  ------- BOT
     *
     * by fitting things onto 2M regions we reduce the number of entries the pagetable requires
     */

    u64 end_of_loaded_sections = (u64)&__ld_end_sections;

    /* we align the stack up to the nearest 2M */
    BOT_OF_STACK_PA = ALIGN_UP(end_of_loaded_sections, PMD_SHIFT);
    TOP_OF_STACK_PA = BOT_OF_STACK_PA + 2*MAX_CPUS*STACK_SIZE;

    /* we allocate between 12.5% of DRAM up to 64 MiB max for heap space */
    TOTAL_HEAP = MIN(64*MiB, ((TOP_OF_MEM - BOT_OF_MEM) / 8));
    BOT_OF_HEAP = TOP_OF_STACK_PA;
    TOP_OF_HEAP = BOT_OF_HEAP + ALIGN_UP(TOTAL_HEAP, PAGE_SHIFT);

    /* we then allocate up to 32 MiB for the pagetables */
    TOTAL_TABLE_SPACE = MIN(32*MiB, ((TOP_OF_MEM - BOT_OF_MEM) / 8));
    BOT_OF_PTABLES = TOP_OF_HEAP;
    TOP_OF_PTABLES = BOT_OF_PTABLES + ALIGN_UP(TOTAL_TABLE_SPACE, PAGE_SHIFT);

    /* finally we allocate whatever is left for the actual test data variables
     */
    BOT_OF_TESTDATA = ALIGN_UP(TOP_OF_PTABLES, PMD_SHIFT);
    TOP_OF_TESTDATA = TOP_OF_MEM;

    HARNESS_MMAP = (u64*)(64 * GiB);
    TESTDATA_MMAP = (u64*)(128 * GiB);

    mem_region = dtb_read_ioregion(fdt);
    BOT_OF_IO = ALIGN_TO(mem_region.base, PAGE_SHIFT);
    TOP_OF_IO = ALIGN_UP(mem_region.top, PAGE_SHIFT);

    /* read cache line of *minimum* size */
    u64 ctr = read_sysreg(ctr_el0);
    u64 dminline = (ctr >> 16) & 0b1111;
    __cache_line_size = 4 * (1 << dminline);
    __cache_line_shift = log2(__cache_line_size);

    LEVEL_SIZES[REGION_CACHE_LINE] = __cache_line_size;
    LEVEL_SHIFTS[REGION_CACHE_LINE] = __cache_line_shift;

    u64 mmfr0 = read_sysreg(id_aa64mmfr0_el1);
    u64 asidbits = BIT_SLICE(mmfr0, 7, 4);
    __asid_size = asidbits == 0 ? 8 : 16;

    u64 dczvid = read_sysreg(dczid_el0);
    __dc_zero_allow = (dczvid >> 4) == 0;
    __dc_zero_block_width = 4 * (1 << (dczvid & 0xf));
}

/* dtb location */
char* fdt_load_addr;

#define FDT_BEGIN_NODE (0x00000001)
#define FDT_END_NODE (0x00000002)
#define FDT_PROP (0x00000003)
#define FDT_NOP (0x00000004)
#define FDT_END (0x00000009)

#define FDT_ALIGN(x) (((x) + 3) & (~3))

/** since device data (and the dtb) is loaded before we read the bootargs we cannot know
 * if we're running with --debug yet.
 *
 * So this has to be a compile-time option
 */
#define __DEVICE_DEBUG__ 0
#if __DEVICE_DEBUG__
#define DDEBUG(...) printf(__VA_ARGS__)
#else
#define DDEBUG(...)
#endif

static u32 read_be(char* p) {
  u8* pc = (u8*)p;
  u8 b0 = *pc;
  u8 b1 = *(pc+1);
  u8 b2 = *(pc+2);
  u8 b3 = *(pc+3);
  return ((u32)b0<<24) + ((u32)b1<<16) + ((u32)b2<<8) + (u32)b3;
}

static u64 read_be64(char* p) {
  u8* pc = (u8*)p;

  u8 bs[] = {
    *(pc+7),
    *(pc+6),
    *(pc+5),
    *(pc+4),
    *(pc+3),
    *(pc+2),
    *(pc+1),
    *(pc+0)
  };

  u64 x = 0;
  for (int i = 7; i >= 0; i--) {
    x <<= 8;
    x += bs[i];
    DDEBUG("[read_be64: %p => %d]\n", pc+i, bs[i]);
  }
  return x;
}

char* dtb_read_str(char* fdt, u32 nameoff) {
    fdt_header* hd = (fdt_header*)fdt;
    char* name_start = fdt + read_be((char*)&hd->fdt_off_dt_strings) + nameoff;
    return name_start;
}

/** given a pointer to the start of a pieces,
 * read it and return a pointer to the start of the next
 */
fdt_structure_piece dtb_read_piece(char* p) {
    u32 token = read_be(p);
    fdt_structure_piece piece;
    piece.current = p;
    piece.token = token;
    char* next;
    char* ps;

    fdt_structure_property_header prop;

    switch (token) {
        case FDT_BEGIN_NODE:
            /* skip name */
            ps = p + sizeof(fdt_structure_begin_node_header);
            while (*ps != '\0')
                ps++;
            next = ps + 1;
            break;

        case FDT_END_NODE:
            next = p + 4;
            break;

        case FDT_PROP:
            prop.token = read_be(p);
            prop.len = read_be(p + 4);
            prop.nameoff = read_be(p + 8);
            next = p + sizeof(fdt_structure_property_header) + prop.len;
            break;

        case FDT_END:
            piece.next = NULL;
            return piece;

        default:
            piece.next = NULL;
            return piece;
    }

    piece.next = (char*)FDT_ALIGN((u64)next);
    return piece;
}

fdt_structure_begin_node_header* fdt_find_node(char* fdt, char* node_name) {
    fdt_header* hd = (fdt_header*)fdt;
    char* struct_block = (char*)((u64)fdt + read_be((char*)&hd->fdt_off_dt_struct));
    char* current_node = NULL;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                if (strcmp(current_node, node_name)) {
                    return (fdt_structure_begin_node_header*)struct_block;
                }

            case FDT_END_NODE:
                break;

            case FDT_PROP:
                break;

            case FDT_END:
                return NULL;

            default:
                break;
        }

        struct_block = piece.next;
    }

    printf("! fdt_find_node, reached end of unreachable function ?\n");
    abort();
    return NULL;
}

fdt_structure_begin_node_header* fdt_read_node(char* fdt, fdt_structure_begin_node_header* node, char* node_name) {
    char* top_node_name = node->name;
    char* struct_block = (char*)node;
    char* current_node = NULL;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                if (strcmp(node_name, current_node)) {
                    return (fdt_structure_begin_node_header*)struct_block;
                }
                break;

            case FDT_END_NODE:
                if (namestackdepth == 1) {
                    return NULL;
                }

                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                break;

            case FDT_END:
                return NULL;

            default:
                break;
        }

        struct_block = piece.next;
    }

    printf("! fdt_read_node, could not find node \"%s\" under node \"%s\": never reached FDT_END\n", node_name, top_node_name);
    abort();
    return NULL;
}

fdt_structure_property_header* fdt_read_prop(char* fdt, fdt_structure_begin_node_header* node, char* prop_name) {
    char* top_node_name = node->name;
    char* struct_block = (char*)node;
    char* current_node = NULL;
    char* prop;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                break;

            case FDT_END_NODE:
                if (namestackdepth == 1) {
                    return NULL;
                }

                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                prop = dtb_read_str(fdt, read_be(struct_block+8));
                if (namestackdepth == 1 && strcmp(prop, prop_name)) {
                    return (fdt_structure_property_header*)struct_block;
                }
                break;

            case FDT_END:
                return NULL;

            default:
                break;
        }

        struct_block = piece.next;
    }

    printf("! fdt_read_prop, could not find property \"%s\" for node \"%s\": never reached FDT_END\n", prop_name, top_node_name);
    abort();
    return NULL;
}

fdt_structure_piece fdt_find_node_with_prop_with_index(char* fdt, char* index, char* prop_name, char* expected_value) {
    fdt_header* hd = (fdt_header*)fdt;
    fdt_structure_begin_node_header* curr_header;

    char* struct_block;

    if (index == NULL)
        struct_block = (char*)((u64)fdt + read_be((char*)&hd->fdt_off_dt_struct));
    else
        struct_block = index;

    char* current_node = NULL;
    char* current_prop_name;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                curr_header = (fdt_structure_begin_node_header*)struct_block;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                break;

            case FDT_END_NODE:
                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                current_prop_name = dtb_read_str(fdt, read_be(struct_block+8));
                if (strcmp(prop_name, current_prop_name)) {
                    fdt_structure_property_header* prop = (fdt_structure_property_header*)struct_block;
                    if (strcmp(prop->data, expected_value)) {
                        return (fdt_structure_piece){.current=(char*)curr_header, .token=FDT_PROP, .next=piece.next};;
                    }
                }
                break;

            case FDT_END:
                return (fdt_structure_piece){.current=NULL, .token=FDT_END, .next=NULL};

            default:
                break;
        }

        struct_block = piece.next;
    }

    fail("! fdt_find_node_with_prop, could not find property \"%s\" with required value in any node as never reached FDT_END\n", prop_name);
    return (fdt_structure_piece){.current=NULL, .token=-1, .next=NULL};
}

fdt_structure_property_header* fdt_find_prop(char* fdt, char* node_name, char* prop_name) {
    fdt_header* hd = (fdt_header*)fdt;
    char* struct_block = (char*)((u64)fdt + read_be((char*)&hd->fdt_off_dt_struct));
    char* current_node = NULL;
    char* prop;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                break;

            case FDT_END_NODE:
                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                prop = dtb_read_str(fdt, read_be(struct_block+8));
                if (strcmp(current_node, node_name) && strcmp(prop_name, prop)) {
                    return (fdt_structure_property_header*)struct_block;
                }
                break;

            case FDT_END:
                return NULL;

            default:
                break;
        }

        struct_block = piece.next;
    }

    printf("! fdt_find_prop, could not find property \"%s\" for node \"%s\": never reached FDT_END\n", prop_name, node_name);
    abort();
    return NULL;
}

void fdt_debug_print_node(char* fdt, fdt_structure_begin_node_header* node, int indent) {
    char* struct_block = (char*)node;

    char* current_node = NULL;
    char* prop;
    fdt_structure_property_header* prop_head;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                for (int i = 0; i < indent+namestackdepth; i++)
                    printf("%s", " ");
                printf("%s: \n", current_node);
                break;

            case FDT_END_NODE:
                if (namestackdepth == 1)
                    return;

                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                prop_head = (fdt_structure_property_header*)struct_block;
                prop = dtb_read_str(fdt, read_be((char*)&prop_head->nameoff));
                for (int i = 0; i < indent+namestackdepth; i++)
                    printf("%s", " ");
                printf(" %s : ", prop);
                char hex_out[1024];
                int len = read_be((char*)&prop_head->len);
                len = len < 100 ? len : 100;
                dump_hex(hex_out, prop_head->data, len);
                printf("%s", hex_out);
                printf("\n");
                break;

            case FDT_END:
                return;

            default:
                break;
        }

        struct_block = piece.next;
    }
}

void fdt_debug_print_all(char* fdt) {
    fdt_header* hd = (fdt_header*)fdt;
    char* struct_block = (char*)((u64)fdt + read_be((char*)&hd->fdt_off_dt_struct));
    char* current_node = NULL;
    char* prop;
    fdt_structure_property_header* prop_head;

    char* namestack[100];  /* max-depth = 100 */
    int namestackdepth = 0;

    while (struct_block != NULL) {
        fdt_structure_piece piece = dtb_read_piece(struct_block);
        switch (piece.token) {
            case FDT_BEGIN_NODE:
                namestack[namestackdepth] = current_node;
                namestackdepth++;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                for (int i = 0; i < namestackdepth; i++)
                    printf("%s", " ");
                printf("%s: \n", current_node);
                break;

            case FDT_END_NODE:
                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                prop_head = (fdt_structure_property_header*)struct_block;
                prop = dtb_read_str(fdt, read_be((char*)&prop_head->nameoff));
                for (int i = 0; i < namestackdepth; i++)
                    printf("%s", " ");
                printf(" %s : ", prop);
                char hex_out[1024];
                int orig_len = read_be((char*)&prop_head->len);
                int len = orig_len < 100 ? orig_len : 100;
                dump_hex(hex_out, prop_head->data, len);
                printf("(%d) %s", orig_len, hex_out);
                if (orig_len >= 100){
                    printf("!\n");
                }
                printf("\n");
                break;

            case FDT_END:
                printf("<FDT_END>\n");
                return;

            default:
                printf("<unknown token %lx>\n", piece.token);
                break;
        }

        struct_block = piece.next;
    }
}

void dtb_check_psci(char* fdt) {
    fdt_structure_begin_node_header* psci = fdt_find_node(fdt, "psci");

    if (psci == NULL) {
        fail("FDT: for enable-method \"psci\" expected a psci node.\n");
    }
}

void dtb_read_cpu_enable(char* fdt) {
    if (fdt == NULL) {
        /* assume PSCI if no DTB */
        boot_data.kind = BOOT_KIND_PSCI;
        return;
    }

    fdt_structure_begin_node_header* cpus = fdt_find_node(fdt, "cpus");

    fdt_structure_begin_node_header* cpu0 = fdt_read_node(fdt, cpus, "cpu@0");
    fdt_structure_begin_node_header* cpu1 = fdt_read_node(fdt, cpus, "cpu@1");
    fdt_structure_begin_node_header* cpu2 = fdt_read_node(fdt, cpus, "cpu@2");
    fdt_structure_begin_node_header* cpu3 = fdt_read_node(fdt, cpus, "cpu@3");

    if (cpu0 == NULL) {
        fail("Malformed dtb: no cpu@0 node\n");
    } else if (cpu1 == NULL) {
        fail("FDT: Could not find CPU 1.\n");
    } else if (cpu2 == NULL) {
        fail("FDT: Could not find CPU 2.\n");
    } else if (cpu3 == NULL) {
        fail("FDT: Could not find CPU 3.\n");
    }

    fdt_structure_property_header* cpu0_enable = fdt_read_prop(fdt, cpu0, "enable-method");
    if (cpu0_enable == NULL) {
        fail("FDT: CPU0 did not have an enable-method property.\n");
    }

    if (strcmp(cpu0_enable->data, "psci")) {
      DDEBUG("Use PSCI to boot\n");
      boot_data.kind = BOOT_KIND_PSCI;
    } else if (strcmp(cpu0_enable->data, "spin-table")) {
      DDEBUG("Using SPIN-TABLE to boot\n");

      boot_data.kind = BOOT_KIND_SPIN;
      fdt_structure_property_header* cpu0_rel_addr = fdt_read_prop(fdt, cpu0, "cpu-release-addr");
      fdt_structure_property_header* cpu1_rel_addr = fdt_read_prop(fdt, cpu1, "cpu-release-addr");
      fdt_structure_property_header* cpu2_rel_addr = fdt_read_prop(fdt, cpu2, "cpu-release-addr");
      fdt_structure_property_header* cpu3_rel_addr = fdt_read_prop(fdt, cpu3, "cpu-release-addr");

      DDEBUG("CPU release-addr(s)  : %p %p %p %p\n", cpu0_rel_addr, cpu1_rel_addr, cpu2_rel_addr, cpu3_rel_addr);

      boot_data.spin_base[0] = read_be64(cpu0_rel_addr->data);
      boot_data.spin_base[1] = read_be64(cpu1_rel_addr->data);
      boot_data.spin_base[2] = read_be64(cpu2_rel_addr->data);
      boot_data.spin_base[3] = read_be64(cpu3_rel_addr->data);
    } else {
        fail("FDT: (cpu@0) cpu-enable expected \"psci\" but got \"%s\"\n", cpu0_enable->data);
    }
}


char* dtb_bootargs(void* fdt) {
    /* read CPU enable methods */
    dtb_read_cpu_enable(fdt);

    if (fdt == NULL) {
        return "";
    }

    /* read cmd-line args passed as bootargs by QEMU */
    fdt_structure_property_header* prop = fdt_find_prop(fdt, "chosen", "bootargs");

    if (prop == NULL)
        return "";

    return prop->data;
}

dtb_mem_t dtb_read_memory(void* fdt) {
    if (fdt == NULL) {
        /* if no given dtb then return default allocation region */
        u64 end_of_loaded_sections = (u64)&__ld_end_sections;
        return (dtb_mem_t){end_of_loaded_sections, 1*GiB, end_of_loaded_sections+1*GiB};
    }

#if __DEVICE_DEBUG__
    fdt_debug_print_all(fdt);
#endif

    /* find the first node with device_type: "memory" */
    fdt_structure_piece piece = fdt_find_node_with_prop_with_index(fdt, NULL, "device_type", "memory");
    fdt_structure_begin_node_header* node = (fdt_structure_begin_node_header*)piece.current;
    fdt_structure_property_header* prop = fdt_read_prop(fdt, node, "reg");

    /* check no other memory nodes (unsupported for now ...) */
    piece = fdt_find_node_with_prop_with_index(fdt, piece.next, "device_type", "memory");
    if (piece.current != NULL) {
        fail("! dtb_read_memory: only expected 1 memory region in dtb, got '%s' too\n",
            ((fdt_structure_begin_node_header*)piece.current)->name
        );
    }

    u32 len = read_be((char*)&prop->len);
    u64 base;
    u64 size;

    /* its reg is stored as big endian {u64_base, u64_size} */
    if (len == 16) {
      u32 blocks[4];
      for (int i = 0; i < 4; i++) {
        blocks[i] = read_be(prop->data + i*4);
      }
      base = (u64)blocks[0] << 32 | blocks[1];
      size = (u64)blocks[2] << 32 | blocks[3];
    } else if (len == 8) {
      /* stored as {u32_base, u32_size} */
      u32 blocks[2];
      for (int i = 0; i < 2; i++) {
        blocks[i] = read_be(prop->data + i*4);
      }

      base = (u64)blocks[0];
      size = (u64)blocks[1];
    } else {
      fail("! dtb_read_memory: unsupported size (%d) for memory node reg\n", prop->len);
    }

    return (dtb_mem_t){base, size, base+size};
}

dtb_mem_t dtb_read_ioregion(void* fdt) {
    if (fdt == NULL) {
        /* if no DTB, assume rpi3 +0x3F00_0000 ... */
        return (dtb_mem_t){0x3F000000UL, PAGE_SIZE, 0x3F001000UL};
    }

    /* look for pl011@9000000
    * which is what mach-virt adds to the DTB
    */
    fdt_structure_begin_node_header* pl011 = fdt_find_node(fdt, "pl011@9000000");
    if (pl011 != NULL)
        return (dtb_mem_t){0x9000000, PAGE_SIZE, 0x9001000};

    /* look for SoC-specific settings */
    fdt_structure_begin_node_header* soc = fdt_find_node(fdt, "soc");
    if (soc == NULL)
        fail("! unsupported architecture: no /soc or /pl011@9000000 nodes in dtb\n");

    /* now check to see if we have a serial @ 0x7e215040 */
    fdt_structure_begin_node_header* serial = fdt_read_node(fdt, soc, "serial@7e215040");

    if (serial == NULL)
        fail("! unsupported architecture: no /pl011@9000000 /soc/serial@7e215040 nodes in dtb\n");

    return (dtb_mem_t){0x3F000000UL, PAGE_SIZE, 0x3F001000UL};
}