#include <stdint.h>

#include "lib.h"
#include "argdef.h"

/* global configuration options + default values */
uint64_t NUMBER_OF_RUNS = 10000UL;
uint8_t ENABLE_PGTABLE = 1;  /* start enabled */
uint8_t ENABLE_PERF_COUNTS = 0;

uint8_t ENABLE_RESULTS_HIST = 1;

uint8_t VERBOSE = 1;  /* start verbose */
uint8_t TRACE = 0;
uint8_t DEBUG = 0;

uint8_t ONLY_SHOW_MATCHES = 0;

char* collected_tests[100];
int   collected_tests_count;

sync_type_t LITMUS_SYNC_TYPE = SYNC_ALL;
aff_type_t LITMUS_AFF_TYPE = AFF_RAND;
shuffle_type_t LITMUS_SHUFFLE_TYPE = SHUF_RAND;
concretize_type_t LITMUS_CONCRETIZATION_TYPE = CONCRETE_RANDOM;
char LITMUS_CONCRETIZATION_CFG[1024];
litmus_runner_type_t LITMUS_RUNNER_TYPE = RUNNER_EPHEMERAL;

char* sync_type_to_str(sync_type_t ty) {
  switch (ty) {
    case SYNC_NONE:
      return "none";
    case SYNC_ALL:
      return "all";
    case SYNC_ASID:
      return "asid";
    case SYNC_VA:
      return "va";
    default:
      return "unknown";
  }
}

char* aff_type_to_str(aff_type_t ty) {
  switch (ty) {
    case AFF_NONE:
      return "none";
    case AFF_RAND:
      return "rand";
    default:
      return "unknown";
  }
}

char* shuff_type_to_str(shuffle_type_t ty) {
  switch (ty) {
    case SHUF_NONE:
      return "none";
    case SHUF_RAND:
      return "rand";
    default:
      return "unknown";
  }
}

char* concretize_type_to_str(concretize_type_t ty) {
  switch (ty) {
    case CONCRETE_LINEAR:
      return "linear";
    case CONCRETE_RANDOM:
      return "random";
    case CONCRETE_FIXED:
      return "fixed";
    default:
      return "unknown";
  }
}

char* runner_type_to_str(litmus_runner_type_t ty) {
  switch (ty) {
    case RUNNER_ARRAY:
      return "array";
    case RUNNER_SEMI_ARRAY:
      return "semi";
    case RUNNER_EPHEMERAL:
      return "ephemeral";
    default:
      return "unknown";
  }
}

static void help(char* opt) {
  if (opt == NULL || *opt == '\0')
    display_help_and_quit(&ARGS);
  else
    display_help_for_and_quit(&ARGS, opt);
}

static void version(char* opt) {
  printf("%s\n", version_string());
  abort();
}

static void __print_id_cpu(int cpu, void* arg) {
  printf("-- ID REGISTERS CPU%d --\n", cpu);
  //printf("CCSIDR2_EL1: %p\n", read_sysreg(ccsidr2_el1));
  printf("CCSIDR_EL1: %p\n", read_sysreg(ccsidr_el1));
  printf("CLIDR_EL1: %p\n", read_sysreg(clidr_el1));
  printf("CSSELR_EL1: %p\n", read_sysreg(csselr_el1));
  printf("CTR_EL0: %p\n", read_sysreg(ctr_el0));
  printf("DCZID_EL0: %p\n", read_sysreg(dczid_el0));
  //printf("GMID_EL1: %p\n", read_sysreg(gmid_el1));
  printf("ID_AA64AFR0_EL1: %p\n", read_sysreg(id_aa64afr0_el1));
  printf("ID_AA64AFR1_EL1: %p\n", read_sysreg(id_aa64afr1_el1));
  printf("ID_AA64DFR0_EL1: %p\n", read_sysreg(id_aa64dfr0_el1));
  printf("ID_AA64DFR1_EL1: %p\n", read_sysreg(id_aa64dfr1_el1));
  printf("ID_AA64ISAR0_EL1: %p\n", read_sysreg(id_aa64isar0_el1));
  printf("ID_AA64ISAR1_EL1: %p\n", read_sysreg(id_aa64isar1_el1));
  printf("ID_AA64MMFR0_EL1: %p\n", read_sysreg(id_aa64mmfr0_el1));
  printf("ID_AA64MMFR1_EL1: %p\n", read_sysreg(id_aa64mmfr1_el1));
  //printf("ID_AA64MMFR2_EL1: %p\n", read_sysreg(id_aa64mmfr2_el1));
  printf("ID_AA64PFR0_EL1: %p\n", read_sysreg(id_aa64pfr0_el1));
  printf("ID_AA64PFR1_EL1: %p\n", read_sysreg(id_aa64pfr1_el1));
  printf("ID_AFR0_EL1: %p\n", read_sysreg(id_afr0_el1));
  printf("ID_DFR0_EL1: %p\n", read_sysreg(id_dfr0_el1));
  //printf("ID_DFR1_EL1: %p\n", read_sysreg(id_dfr1_el1));
  printf("ID_ISAR0_EL1: %p\n", read_sysreg(id_isar0_el1));
  printf("ID_ISAR1_EL1: %p\n", read_sysreg(id_isar1_el1));
  printf("ID_ISAR2_EL1: %p\n", read_sysreg(id_isar2_el1));
  printf("ID_ISAR3_EL1: %p\n", read_sysreg(id_isar3_el1));
  printf("ID_ISAR4_EL1: %p\n", read_sysreg(id_isar4_el1));
  printf("ID_ISAR5_EL1: %p\n", read_sysreg(id_isar5_el1));
  //printf("ID_ISAR6_EL1: %p\n", read_sysreg(id_isar6_el1));
  printf("ID_MMFR0_EL1: %p\n", read_sysreg(id_mmfr0_el1));
  printf("ID_MMFR1_EL1: %p\n", read_sysreg(id_mmfr1_el1));
  printf("ID_MMFR2_EL1: %p\n", read_sysreg(id_mmfr2_el1));
  printf("ID_MMFR3_EL1: %p\n", read_sysreg(id_mmfr3_el1));
  printf("ID_MMFR4_EL1: %p\n", read_sysreg(id_mmfr4_el1));
  //printf("ID_MMFR5_EL1: %p\n", read_sysreg(id_mmfr5_el1));
  printf("ID_PFR0_EL1: %p\n", read_sysreg(id_pfr0_el1));
  printf("ID_PFR1_EL1: %p\n", read_sysreg(id_pfr1_el1));
  //printf("ID_PFR2_EL1: %p\n", read_sysreg(id_pfr2_el1));
  printf("MIDR_EL1: %p\n", read_sysreg(midr_el1));
  printf("MPIDR_EL1: %p\n", read_sysreg(mpidr_el1));
  printf("REVIDR_EL1: %p\n", read_sysreg(revidr_el1));
}

static void device_ident(char* opt) {
  uint64_t midr = read_sysreg(midr_el1);
  uint64_t rev = midr & BITMASK(1 + 3 - 0);
  uint64_t partnum = (midr >> 4) & BITMASK(1 + 15 - 4);
  uint64_t variant = (midr >> 20) & BITMASK(1 + 23 - 20);
  uint64_t impl = (midr >> 24) & BITMASK(1 + 31 - 24);

  const char* impl_names[0x100] = {
    [0x00] = "Reserved for software use Ampere Computing",
    [0xC0] = "Arm Limited",
    [0x41] = "Broadcom Corporation Cavium Inc.",
    [0x42] = "Digital Equipment Corporation",
    [0x43] = "Fujitsu Ltd.",
    [0x44] = "Infineon Technologies AG",
    [0x46] = "Motorola or Freescale Semiconductor Inc.",
    [0x49] = "NVIDIA Corporation",
    [0x4D] = "Applied Micro Circuits Corporation",
    [0x4E] = "Qualcomm Inc.",
    [0x50] = "Marvell International Ltd.",
    [0x51] = "Intel Corporation",
  };

  printf("-- MIDR --\n");
  printf("Implementor: %s\n", impl_names[impl]);
  printf("Variant: %p\n", variant);
  printf("Revision: %p\n", rev);
  printf("PartNum: %p\n", partnum);

  ENABLE_PGTABLE = 0; /* pgtable may not be setup yet, so dont try use it on the other booted CPUs */
  ensure_cpus_on();
  for (int cpu = 0; cpu < NO_CPUS; cpu++)
    run_on_cpu(cpu, __print_id_cpu, NULL);
}

static void n(char* x) {
  int Xn = atoi(x);
  NUMBER_OF_RUNS = Xn;
}

static void s(char* x) {
  int Xn = atoi(x);
  INITIAL_SEED = Xn;
}

static void show(char* x) {
  display_help_show_tests();
}

static void q(char* x) {
  if (x != NULL && *x != '\0') {
    fail("-q/--quiet did not expect an argument.\n");
  }

  VERBOSE = 0;
  DEBUG = 0;
  TRACE = 0;
}

static void conc_cfg(char* x) {
  valloc_memcpy(LITMUS_CONCRETIZATION_CFG, x, strlen(x));
}

argdef_t ARGS = (argdef_t){
  .args=(const argdef_arg_t*[]){
    OPT(
      "-h",
      "--help",
      help,
      "display this help text and quit\n"
      "\n"
      "displays help text.\n"
      "--help=foo will display detailed help for foo.",
      .show_help_both=1
    ),
    OPT(
      "-V",
      "--version",
      version,
      "display version information and quit\n"
      "\n"
      "displays full version info\n"
    ),
    OPT(
      NULL,
      "--id",
      device_ident,
      "display device identification information\n"
      "\n"
      "shows the information for the current device, and all ID registers."
    ),
    OPT(
      NULL,
      "--show",
      show,
      "show list of tests and quit\n"
      "\n"
      "displays the complete list of the compiled tests and their groups, then quits.",
      .only_action=1
    ),
    OPT(
      "-n",
      NULL,
      n,
      "number of runs per test\n"
      "\n"
      "sets the number of runs per test\n"
      "X must be an integer (default: 10k).\n"
      "Examples: \n"
      " ./litmus.exe -n300\n"
      " ./litmus.exe -n10k\n"
      " ./litmus.exe -n1M\n"
      "\n"
      "Note that currently 1M is likely to fail due to over-allocation of results.\n"
    ),
    FLAG(
      "-p",
      "--pgtable",
      ENABLE_PGTABLE,
      "enable/disable pagetable tests\n"
    ),
    FLAG(
      NULL,
      "--perf",
      ENABLE_PERF_COUNTS,
      "enable/disable performance tests\n"
    ),
    FLAG(
      "-t",
      "--trace",
      TRACE,
      "enable/disable tracing\n"
    ),
    FLAG(
      "-d",
      "--debug",
      DEBUG,
      "enable/disable debugging\n"
    ),
    FLAG(
      NULL,
      "--verbose",
      VERBOSE,
      "enable/disable verbose output\n"
    ),
    OPT(
      "-q",
      "--quiet",
      q,
      "quiet mode\n"
      "\n"
      "disables verbose/trace and debug output.\n",
      .only_action=1
    ),
    FLAG(
      NULL,
      "--hist",
      ENABLE_RESULTS_HIST,
      "enable/disable results histogram collection\n"
    ),
    OPT(
      "-s",
      "--seed",
      s,
      "initial seed\n"
      "\n"
      "at the beginning of each test, a random seed is selected.\n"
      "this seed can be forced with --seed=12345 which ensures some level of determinism."
    ),
    ENUMERATE(
      "--tlbsync",
      LITMUS_SYNC_TYPE,
      sync_type_t,
      4,
      ARR((const char*[]){"none", "asid", "va", "all"}),
      ARR((sync_type_t[]){SYNC_NONE, SYNC_ASID, SYNC_VA, SYNC_ALL}),
      "type of tlb synchronization\n"
      "\n"
      "none:  no synchronization of the TLB (incompatible with --pgtable).\n"
      "all: always flush the entire TLB in-between tests.\n"
      "va: (EXPERIMENTAL) only flush test data VAs\n"
      "asid: (EXPERIMENTAL) assign each test run an ASID and only flush that \n"
    ),
    ENUMERATE(
      "--aff",
      LITMUS_AFF_TYPE,
      aff_type_t,
      2,
      ARR((const char*[]){"none", "rand"}),
      ARR((aff_type_t[]){AFF_NONE, AFF_RAND}),
      "type of affinity control\n"
      "\n"
      "none: Thread 0 is pinned to CPU0, Thread 1 to CPU1 etc.\n"
      "rand: Thread 0 is pinned to vCPU0, etc.  vCPUs may migrate between CPUs"
      " in-between tests.\n"
    ),
    ENUMERATE(
      "--shuffle",
      LITMUS_SHUFFLE_TYPE,
      shuffle_type_t,
      2,
      ARR((const char*[]){"none", "rand"}),
      ARR((shuffle_type_t[]){SHUF_NONE, SHUF_RAND}),
      "type of shuffle control\n"
      "\n"
      "controls the order of access to allocated pages\n"
      "shuffling the indexes more should lead to more interesting caching results.\n"
      "\n"
      "none: access pages in-order\n"
      "rand: access pages in random order\n"
    ),
    ENUMERATE(
      "--concretize",
      LITMUS_CONCRETIZATION_TYPE,
      concretize_type_t,
      3,
      ARR((const char*[]){"linear", "random", "fixed"}),
      ARR((concretize_type_t[]){CONCRETE_LINEAR, CONCRETE_RANDOM, CONCRETE_FIXED}),
      "test concretization algorithml\n"
      "\n"
      "controls the memory layout of generated concrete tests\n"
      "\n"
      "linear: allocate each var as a fixed shape and walk linearly over memory\n"
      "random: allocate randomly\n"
      "fixed: always use the same address, random or can be manually picked via --config-concretize\n"
    ),
    OPT(
      NULL,
      "--config-concretize",
      conc_cfg,
      "concretization-specific configuration\n"
      "\n"
      "the format differs depending on the value of --concretize:\n"
      "for random, linear: do nothing.\n"
      "for fixed:\n"
      "format:  [<var>=<value]*\n"
      "example: \n"
      "  --config-concretize=\"x=0x1234,y=0x5678,c=0x9abc\"\n"
      "  places x at 0x1234, y at 0x5678 and z at 0x9abc.\n"
    ),
    NULL
  }
};

void read_args(int argc, char** argv) {
  argparse_read_args(&ARGS, argc, argv);
}