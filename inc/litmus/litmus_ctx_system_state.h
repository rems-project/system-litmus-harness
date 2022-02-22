#ifndef LITMUS_CTX_SYSTEM_STATE_H
#define LITMUS_CTX_SYSTEM_STATE_H


/** each test can setup a set of system registers
 * for use in the test
 */
typedef struct {
  /** value for MAIR_EL1.Attr7 */
  u8 enable_mair;
  u64 mair_attr7;
} init_system_state_t;

#endif /* LITMUS_CTX_SYSTEM_STATE_H */