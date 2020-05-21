#include "lib.h"

void init_device(void) {
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

fdt_structure_property_header* fdt_read_prop(char* fdt, char* node_name, char* prop_name) {
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

    printf("! fdt_read_prop, could not find property \"%s\" for node \"%s\": never reached FDT_END\n", prop_name, node_name);
    abort();
    return NULL;
}

char* dtb_bootargs(void* fdt) {
    fdt_structure_property_header* prop = fdt_read_prop(fdt, "chosen", "bootargs");

    if (prop == NULL)
        return "";

    return prop->data;
}
