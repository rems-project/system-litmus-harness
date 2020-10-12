#ifndef VMM_TABLES_H
#define VMM_TABLES_H
#include "bitwise.h"

#define OFFS0(va) BIT_SLICE(va, 48, 39)
#define OFFS1(va) BIT_SLICE(va, 38, 30)
#define OFFS2(va) BIT_SLICE(va, 29, 21)
#define OFFS3(va) BIT_SLICE(va, 20, 12)

#define OFFSBOT(level) \
    (level == 0 ? 39 \
        : (level == 1 ? 30 \
            : (level == 2 ? 21 : 12)))

#define OFFSTOP(level) \
    (level == 0 ? 48 \
        : (level == 1 ? 38 \
            : (level == 2 ? 29 : 20)))


#define OFFS(va, lvl) BIT_SLICE(va, OFFSTOP(lvl), OFFSBOT(lvl))

#endif /* VMM_TABLES_H */
