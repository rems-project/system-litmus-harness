/** linear concretization algorithm
 *
 *
 * each var can OWN a region of memory
 *      or be PINNED within another region
 *
 * each region can optionally have a RELATION with another variable
 *
 * for example, for a litmus test you might want:
 *  X OWNS a PAGE (4K region)
 *  Y PINNED to X's page (so in the same 4k region)
 *  Z OWNS a PAGE (so different 4K region to X and Y)
 *  Z RELATES to X's PAGE offset (so shares the same last 12 bits)
 *
 *
 * To allocate VAs we follow the simple following algorithm:
 *  1. Pick one of the variables as the "root" at give it offset 0
 *  2. For each var V that owns a region, in order of the size of the region
 *      a) if V RELATES to V' and V' has been allocated an offset
 *        i) raise the current offset to match the last N bits of V'
 *      b) put V at the current offset
 *      c) for each var V' pinned to V, in order of size of the pin (closest->furthest)
 *         i) increment current offset by size of the level of pin below the current
 *                (e.g. if at offset 0x1000 and pinned to the PMD, then incremenet by a PAGE)
 *         ii) if V' RELATES to V''  and V'' has been allocated an offset
 *            1) raise the current offset to match the last N bits of V''
 *            2) if the current offset pushes V' outside the pin, fail.
 *         iii) put V' at current offset
 *      d) move offset to 1+ end of V's region
 *
 * 3. Set start=0
 * 4. For each run i
 *    a) for each var V
 *      i) set VA for V on run i to start + offset of V
 *    b) incremenet start up to the next available place
 *        (such that the next set of variables will not collide with previous ones)
 */

#include "lib.h"

void* concretize_linear_init(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  return NULL;
}

void concretize_linear_finalize(test_ctx_t* ctx, const litmus_test_t* cfg, void* st) {
  /* nop */
}

void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int no_runs) {
  /* nop */
}
