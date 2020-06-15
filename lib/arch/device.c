#include "lib.h"

void init_device(void) {
    NO_CPUS = 4;

    /* some of these are hard-coded for now but should really be read from the dtb and lds */
    TOP_OF_MEM = 0x48000000UL;
    TOP_OF_STACK = (uint64_t)&stacktop;
    BOT_OF_STACK = (uint64_t)&data_end;
    TOTAL_HEAP = TOP_OF_MEM - TOP_OF_STACK;
    BOT_OF_HEAP = TOP_OF_MEM - TOTAL_HEAP;
    TOP_OF_TEXT = (uint64_t)&text_end;
    TOP_OF_DATA = BOT_OF_STACK;
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
