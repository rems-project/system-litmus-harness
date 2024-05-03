#pragma once

#define CPTR_No_Trap_SVE (0 << 10)
#define CPTR (CPTR_No_Trap_SVE)

#define SCR_Lower_Use_AArch64 (1 << 10)
#define SCR_Enable_HVC (1 << 8)
#define SCR_Enable_SMC (1 << 7) /* required for e.g. PSCI */
#define SCR_RES1 (3 << 4)
#define SCR_NonSecure (1)
#define SCR (SCR_Lower_Use_AArch64 | SCR_Enable_HVC | SCR_Enable_SMC | SCR_RES1 | SCR_NonSecure)

#define ACTLR_Enable_Access_CPUECTLR (1 << 1)
#define ACTLR (ACTLR_Enable_Access_CPUECTLR)

#define CPUECTLR_Data_Memory_Coherent (1 << 6)
#define CPUECTLR (CPUECTLR_Data_Memory_Coherent)
#define CPUECTLR_EL1 S3_1_C15_C2_1

#define SCTLR_RES1 ((3 << 28) | (3 << 22) | (1 << 18) | (1 << 16) | (1 << 11) | (3 << 4))
#define SCTLR (SCTLR_RES1)

#define SPSR_Drop_To_EL2 (2 << 2)
#define SPSR_Use_SP_EL0 0
#define SPSR_DAIF (15 << 6)
#define SPSR (SPSR_DAIF | SPSR_Drop_To_EL2 | SPSR_Use_SP_EL0)