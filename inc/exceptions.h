#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

/* defined in vector_table.S */
extern uint64_t el1_exception_vector_table_p0;

/* Enum of vector table entries
 * Stored in order, aligned at 0x20 boundries
 */
enum vec_entries {
  VEC_EL1T_SYNC,
  VEC_EL1T_IRQ,
  VEC_EL1T_FIQ,
  VEC_EL1T_ERROR,
  VEC_EL1H_SYNC,
  VEC_EL1H_IRQ,
  VEC_EL1H_FIQ,
  VEC_EL1H_ERROR,
  VEC_EL0_SYNC_64,
  VEC_EL0_IRQ_64,
  VEC_EL0_FIQ_64,
  VEC_EL0_ERROR_64,
  VEC_EL0_SYNC_32,
  VEC_EL0_IRQ_32,
  VEC_EL0_FIQ_32,
  VEC_EL0_ERROR_32,
};

/* useful ECs */
#define EC_SVC64     0x15  /* SVC from AArch64 */
#define EC_IABT_EL0  0x20  /* Instruction Abort */
#define EC_IABT_EL1  0x21  /* Instruction Abort */
#define EC_PC_ALIGN  0x22  /* PC Alignment Fault */
#define EC_DABT_EL0  0x24  /* Data Abort */
#define EC_DABT_EL1  0x25  /* Data Abort */

typedef struct {
    uint64_t gpr[31];
    uint64_t sp;
} regvals_t;

/* Exception Vectors
 *
 *  SVC has default handlers provided by the framework:
 *    SVC #10  will drop to EL0
 *    SVC #11  will raise to EL1
 *    SVC #12  will place CurrentEL in X0
 *
 *  Other handlers can be installed with set_svc_handler(svc_no, ptr_to_func)
 */

void* default_handler(uint64_t vec, uint64_t esr, regvals_t* regs);
typedef void* exception_vector_fn(uint64_t esr, regvals_t* regs);

void set_handler(uint64_t vec, uint64_t ec,  exception_vector_fn* fn);
void reset_handler(uint64_t vec, uint64_t ec);
void* handle_exception(uint64_t vec, uint64_t esr, regvals_t* regs);

void set_svc_handler(uint64_t svc_no, exception_vector_fn* fn);
void reset_svc_handler(uint64_t svc_no);

void set_pgfault_handler(uint64_t va, exception_vector_fn* fn);
void reset_pgfault_handler(uint64_t va);

/* not sure if these are a good idea or whether should just have boundaries at function calls */

void drop_to_el0(void);
void raise_to_el1(void);

/**
 * hot swapping
 *
 * during a litmus test we will want to replace the vector table entry with
 * some literal assembly without synchronization
 *
 * uint32_t* old = hotswap_exception(0x200, (uint32_t){ ... }) will replace
 *  VBAR+0x200[0:31] with the contents of the ... array
 * and return a pointer to an array storing the old contents of VBAR+0x200
 *
 * restore_hotswapped_exception(0x200, old) will then restore the vector table back
 * to its original state.
 */
uint32_t* hotswap_exception(uint64_t vector_slot, uint32_t data[32]);
void restore_hotswapped_exception(uint64_t vector_slot, uint32_t* ptr);
#endif /* EXCEPTIONS_H */
