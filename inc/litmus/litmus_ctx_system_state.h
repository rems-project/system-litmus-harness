#ifndef LITMUS_CTX_SYSTEM_STATE_H
#define LITMUS_CTX_SYSTEM_STATE_H

#include <stdint.h>

/** each test can setup a set of system registers
 * for use in the test
 */
typedef struct {
  /** value for MAIR_EL1.Attr7 */
  uint8_t enable_mair;
  uint64_t mair_attr7;
} init_system_state_t;

#endif /* LITMUS_CTX_SYSTEM_STATE_H */