#include "lib.h"

uint64_t __cache_line_size;  /* cache line of maximum size */
uint64_t __cache_line_shift;  /* log2(__cache_line_size) */

uint8_t __dc_zero_allow;  /* DC ZVA allow ? */
uint64_t __dc_zero_block_width;  /* size of DC ZVA block */

uint64_t __asid_size;  /* number of ASID bits */

extern char* __ld_begin_text;
extern char* __ld_end_text;
extern char* __ld_begin_reloc;
extern char* __ld_end_reloc;
extern char* __ld_begin_stack;
extern char* __ld_end_stack;
extern char* __ld_end_sections;
extern char* __ld_begin_data;
extern char* __ld_end_data;

void init_device(void* fdt) {
    NO_CPUS = 4;

    /* read the memory region from the dtb */
    dtb_mem_t mem = dtb_read_memory(fdt);
    TOP_OF_MEM = mem.top;
    BOT_OF_MEM = mem.base;
    TOTAL_MEM = mem.size;

    /* read regions from linker */
    TOP_OF_TEXT = (uint64_t)&__ld_end_text;
    BOT_OF_HEAP = (uint64_t)&__ld_end_sections;
    BOT_OF_TEXT = (uint64_t)&__ld_begin_text;
    BOT_OF_DATA = (uint64_t)&__ld_begin_data;
    TOP_OF_DATA = (uint64_t)&__ld_end_data;

    /* compute remaining friendly region names */

    /* we allocate 128M for the heap */
    TOTAL_HEAP = 128 * MiB;
    TOP_OF_HEAP = BOT_OF_HEAP + TOTAL_HEAP;

    BOT_OF_STACK_PA = (uint64_t)&__ld_begin_stack;
    BOT_OF_STACK_PA = ALIGN_UP(BOT_OF_STACK_PA, PMD_SHIFT);
    TOP_OF_STACK_PA = (uint64_t)&__ld_end_stack;
    TOP_OF_STACK_PA = ALIGN_TO(TOP_OF_STACK_PA, PMD_SHIFT);

    BOT_OF_TESTDATA = TOP_OF_HEAP;
    TOP_OF_TESTDATA = TOP_OF_MEM;

    HARNESS_MMAP = (uint64_t*)(64 * GiB);
    TESTDATA_MMAP = (uint64_t*)(128 * GiB);

    /* read cache line of *minimum* size */
    uint64_t ctr = read_sysreg(ctr_el0);
    uint64_t dminline = (ctr >> 16) & 0b1111;
    __cache_line_size = 4 * (1 << dminline);
    __cache_line_shift = log2(__cache_line_size);

    LEVEL_SIZES[REGION_CACHE_LINE] = __cache_line_size;
    LEVEL_SHIFTS[REGION_CACHE_LINE] = __cache_line_shift;

    uint64_t mmfr0 = read_sysreg(id_aa64mmfr0_el1);
    uint64_t asidbits = BIT_SLICE(mmfr0, 7, 4);
    __asid_size = asidbits == 0 ? 8 : 16;

    uint64_t dczvid = read_sysreg(dczid_el0);
    __dc_zero_allow = (dczvid >> 4) == 0;
    __dc_zero_block_width = 4 * (1 << (dczvid & 0xf));
}

/* dtb location */
char* fdt;

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
//#define __DEVICE_DEBUG__ 1
#ifdef __DEVICE_DEBUG__
#define DDEBUG(...) printf(__VA_ARGS__)
#else
#define DDEBUG(...)
#endif

static uint32_t read_be(char* p) {
    uint8_t* pc = (uint8_t*)p;
    uint8_t b0 = *pc;
    uint8_t b1 = *(pc+1);
    uint8_t b2 = *(pc+2);
    uint8_t b3 = *(pc+3);
    return ((uint32_t)b0<<24) + ((uint32_t)b1<<16) + ((uint32_t)b2<<8) + (uint32_t)b3;
}

char* dtb_read_str(char* fdt, uint32_t nameoff) {
    fdt_header* hd = (fdt_header*)fdt;
    char* name_start = fdt + read_be((char*)&hd->fdt_off_dt_strings) + nameoff;
    return name_start;
}

/** given a pointer to the start of a pieces,
 * read it and return a pointer to the start of the next
 */
fdt_structure_piece dtb_read_piece(char* p) {
    uint32_t token = read_be(p);
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

    piece.next = (char*)FDT_ALIGN((uint64_t)next);
    return piece;
}


fdt_structure_begin_node_header* fdt_find_node(char* fdt, char* node_name) {
    fdt_header* hd = (fdt_header*)fdt;
    char* struct_block = (char*)((uint64_t)fdt + read_be((char*)&hd->fdt_off_dt_struct));
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

fdt_structure_begin_node_header* fdt_read_node(fdt_structure_begin_node_header* node, char* node_name) {
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
                if (strcmp(current_node, node_name)) {
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

fdt_structure_property_header* fdt_read_prop(fdt_structure_begin_node_header* node, char* prop_name) {
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
                if (namestackdepth == 1 && strcmp(prop_name, prop)) {
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
        struct_block = (char*)((uint64_t)fdt + read_be((char*)&hd->fdt_off_dt_struct));
    else
        struct_block = index;

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
                curr_header = (fdt_structure_begin_node_header*)struct_block;
                current_node = struct_block+sizeof(fdt_structure_begin_node_header);
                break;

            case FDT_END_NODE:
                namestackdepth--;
                current_node = namestack[namestackdepth];
                break;

            case FDT_PROP:
                prop = dtb_read_str(fdt, read_be(struct_block+8));
                if (strcmp(prop, prop_name)) {
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
    char* struct_block = (char*)((uint64_t)fdt + read_be((char*)&hd->fdt_off_dt_struct));
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
                if (strcmp(node_name, current_node) && strcmp(prop_name, prop)) {
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

void fdt_debug_print_node(fdt_structure_begin_node_header* node, int indent) {
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
    char* struct_block = (char*)((uint64_t)fdt + read_be((char*)&hd->fdt_off_dt_struct));
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
                return;

            default:
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
    fdt_structure_begin_node_header* cpus = fdt_find_node(fdt, "cpus");

    fdt_structure_begin_node_header* cpu0 = fdt_read_node(cpus, "cpu@0");
    fdt_structure_begin_node_header* cpu1 = fdt_read_node(cpus, "cpu@1");
    fdt_structure_begin_node_header* cpu2 = fdt_read_node(cpus, "cpu@2");
    fdt_structure_begin_node_header* cpu3 = fdt_read_node(cpus, "cpu@3");

    if (cpu0 == NULL) {
        fail("Malformed dtb: no cpu@0 node\n");
    } else if (cpu1 == NULL) {
        fail("FDT: Could not find CPU 1.\n");
    } else if (cpu2 == NULL) {
        fail("FDT: Could not find CPU 2.\n");
    } else if (cpu3 == NULL) {
        fail("FDT: Could not find CPU 3.\n");
    }

    fdt_structure_property_header* cpu0_enable = fdt_read_prop(cpu0, "enable-method");
    if (cpu0_enable == NULL) {
        fail("FDT: CPU0 did not have an enable-method property.\n");
    }

    if (strcmp(cpu0_enable->data, "psci")) {
        DDEBUG("Use PSCI to boot\n");
        dtb_check_psci(fdt);
    } else {
        fail("FDT: (cpu@0) cpu-enable expected \"psci\" but got \"%s\"\n", cpu0_enable->data);
    }
}


char* dtb_bootargs(void* fdt) {
#ifdef __DEVICE_DEBUG__
    fdt_debug_print_all(fdt);
#endif

    /* read CPU enable methods */
    dtb_read_cpu_enable(fdt);

    /* read cmd-line args passed as bootargs by QEMU */
    fdt_structure_property_header* prop = fdt_find_prop(fdt, "chosen", "bootargs");

    if (prop == NULL)
        return "";

    return prop->data;
}

dtb_mem_t dtb_read_memory(void* fdt) {
    /* find the first node with device_type: "memory" */
    fdt_structure_piece piece = fdt_find_node_with_prop_with_index(fdt, NULL, "device_type", "memory");
    fdt_structure_begin_node_header* node = (fdt_structure_begin_node_header*)piece.current;
    fdt_structure_property_header* prop = fdt_read_prop(node, "reg");

    /* its reg is stored as big endian {u64_base, u64_size} */
    uint32_t blocks[4];
    for (int i = 0; i < 4; i++) {
      blocks[i] = read_be(prop->data + i*4);
    }
    uint64_t base = (uint64_t)blocks[0] << 32 | blocks[1];
    uint64_t size = (uint64_t)blocks[2] << 32 | blocks[3];

    /* check no other memory nodes (unsupported for now ...) */
    piece = fdt_find_node_with_prop_with_index(fdt, piece.next, "device_type", "memory");
    if (piece.current != NULL) {
        fail("! dtb_read_memory: only expected 1 memory region in dtb, got '%s' too\n",
            ((fdt_structure_begin_node_header*)piece.current)->name
        );
    }

    return (dtb_mem_t){base, size, base+size};
}