#ifndef LITMUS_ASM_H
#define LITMUS_ASM_H

#define HINT(CRmop2) \
  "HINT #" #CRmop2

/* we use a reserved HINT instruction with CRm:op2 == 0b1111_111
 * as a hint at the start/end of litmus blocks */
#define LITMUS_HINT \
  HINT(0b1111111)

/**
 * The Litmus test ASM code should be bracketed
 * with the payload between LITMUS_START_ASM and LITMUS_END_ASM macros
 */
#define LITMUS_START_ASM LITMUS_HINT "\n\t"
#define LITMUS_END_ASM LITMUS_HINT "\n\t"

#endif /* LITMUS_ASM_H */