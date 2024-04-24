
#include "lib.h"
#include "argdef.h"

/* global configuration options + default values */
u64 NUMBER_OF_RUNS = 10000UL;
u64 RUNS_IN_BATCH = 1;
u8 ENABLE_PGTABLE = 1;  /* start enabled */
u8 ENABLE_PERF_COUNTS = 0;
u8 RUN_FOREVER = 0;

u8 ENABLE_RESULTS_HIST = 1;
u8 ENABLE_RESULTS_OUTREG_PRINT = 1;
u8 ENABLE_RESULTS_MISSING_SC_WARNING = 1;

u8 VERBOSE = 1;  /* start verbose */
u8 TRACE = 0;
u8 DEBUG = 0;

u8 ONLY_SHOW_MATCHES = 0;

char* collected_tests[100] = { NULL };
u64 collected_tests_count = 0;

sync_type_t LITMUS_SYNC_TYPE = SYNC_ASID;
aff_type_t LITMUS_AFF_TYPE = AFF_RAND;
shuffle_type_t LITMUS_SHUFFLE_TYPE = SHUF_RAND;
concretize_type_t LITMUS_CONCRETIZATION_TYPE = CONCRETE_RANDOM;
char LITMUS_CONCRETIZATION_CFG[1024] = { '\0' };
litmus_runner_type_t LITMUS_RUNNER_TYPE = RUNNER_EPHEMERAL;

u8 ENABLE_UNITTESTS_CONCRETIZATION_TEST_RANDOM = 1;
u8 ENABLE_UNITTESTS_CONCRETIZATION_TEST_LINEAR = 1;

output_style_t OUTPUT_FORMAT = STYLE_HERDTOOLS;

char* output_style_to_str(output_style_t ty) {
  switch (ty) {
    case STYLE_HERDTOOLS:
      return "herdtools";
    case STYLE_ORIGINAL:
      return "orig";
    default:
      return "unknown";
  }
}

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
  if (opt == NULL || *opt == '\0') {
    display_help_and_quit();
  } else {
    display_help_for_and_quit(&COMMON_ARGS, opt);
    display_help_for_and_quit(THIS_ARGS, opt);
  }

  fail("unknown argument '%s'\n", opt);
}

static void version(void) {
  printf("%s\n", version_string());
  abort();
}

static void __print_id_cpu(int cpu, void* arg) {
  char** outs = arg;
  int i = 0;

#define __IDREG(r) sprintf(outs[i++], #r ": %p", read_sysreg(r))
  __IDREG(CCSIDR_EL1);
  __IDREG(CLIDR_EL1);
  __IDREG(CSSELR_EL1);
  __IDREG(CTR_EL0);
  __IDREG(DCZID_EL0);
  //__IDREG(GMID_EL1);
  __IDREG(ID_AA64AFR0_EL1);
  __IDREG(ID_AA64AFR1_EL1);
  __IDREG(ID_AA64DFR0_EL1);
  __IDREG(ID_AA64DFR1_EL1);
  __IDREG(ID_AA64ISAR0_EL1);
  __IDREG(ID_AA64ISAR1_EL1);
  __IDREG(ID_AA64MMFR0_EL1);
  __IDREG(ID_AA64MMFR1_EL1);
  //__IDREG(ID_AA64MMFR2_EL1);
  __IDREG(ID_AA64PFR0_EL1);
  __IDREG(ID_AA64PFR1_EL1);
  __IDREG(ID_AFR0_EL1);
  __IDREG(ID_DFR0_EL1);
  //__IDREG(ID_DFR1_EL1);
  __IDREG(ID_ISAR0_EL1);
  __IDREG(ID_ISAR1_EL1);
  __IDREG(ID_ISAR2_EL1);
  __IDREG(ID_ISAR3_EL1);
  __IDREG(ID_ISAR4_EL1);
  __IDREG(ID_ISAR5_EL1);
  //__IDREG(ID_ISAR6_EL1);
  __IDREG(ID_MMFR0_EL1);
  __IDREG(ID_MMFR1_EL1);
  __IDREG(ID_MMFR2_EL1);
  __IDREG(ID_MMFR3_EL1);
  __IDREG(ID_MMFR4_EL1);
  //__IDREG(ID_MMFR5_EL1);
  __IDREG(ID_PFR0_EL1);
  __IDREG(ID_PFR1_EL1);
  //__IDREG(ID_PFR2_EL1);
  __IDREG(MIDR_EL1);
  __IDREG(MPIDR_EL1);
  __IDREG(REVIDR_EL1);
}

static void device_ident_and_quit(void) {
  u64 midr = read_sysreg(midr_el1);
  u64 rev = midr & BITMASK(1 + 3 - 0);
  u64 partnum = (midr >> 4) & BITMASK(1 + 15 - 4);
  u64 variant = (midr >> 20) & BITMASK(1 + 23 - 20);
  u64 impl = (midr >> 24) & BITMASK(1 + 31 - 24);

  const char* impl_names[0x100] = {
    [0x00] = "Reserved for software use",
    [0x41] = "Arm Limited",
    [0x42] = "Broadcom Corporation",
    [0x43] = "Cavium Inc",
    [0x44] = "Digital Equipment Corporation",
    [0x46] = "Fujitsu Ltd",
    [0x49] = "Infineon Technologies AG",
    [0x4D] = "Motorola or Freescale Semiconductor Inc",
    [0x4E] = "NVIDIA Corporation",
    [0x50] = "Applied Micro Circuits Corporation",
    [0x51] = "Qualcomm Inc",
    [0x56] = "Marvell International Ltd",
    [0x69] = "Intel Corporation",
    [0xC0] = "Ampere Computing",
  };

  // all arm Cortex part names are of the form 0xDXY
  // see respective cores TRM, look for "MIDR_EL1, Main ID Register"
  // sadly each TRM puts the information in some random different place
  const char *arm_part_names[0x100] = {
    [0x01] = "Cortex-A32",
    [0x02] = "Cortex-A34",
    [0x03] = "Cortex-A53",
    [0x04] = "Cortex-A35",
    [0x05] = "Cortex-A56",
    [0x07] = "Cortex-A57",
    [0x08] = "Cortex-A72",
    [0x09] = "Cortex-A73",
    [0x0A] = "Cortex-A75",
    [0x0B] = "Cortex-A76",
    [0x0C] = "Neoverse N1",
    [0x0D] = "Cortex-A77",
    [0x40] = "Neoverse V1",
    [0x41] = "Cortex-A78",
    [0x44] = "Cortex-X1",
    [0x46] = "Cortex-A510",
    [0x47] = "Cortex-A710",
    [0x48] = "Cortex-X2",
    [0x49] = "Neoverse N2",
    [0x4D] = "Cortex-A715",
    [0x4E] = "Cortex-X3",
    [0x4F] = "Neoverse V2",
    [0x80] = "Cortex-A520",
    [0x81] = "Cortex-A720",
    [0x82] = "Cortex-X4",
  };

  const char *part_name = "unknown, refer to Implementor documentation.";

  // Arm
  if (impl == 'A') {
    // check it's a cortex-y thing
    if ((partnum & 0xD00) == 0xD00) {
      int arm_part = partnum - 0xD00;
      if (arm_part < 0x100 && arm_part_names[arm_part]) {
        part_name = arm_part_names[arm_part];
      }
    }
  }

  printf("Implementor: '%c' (%s)\n", impl, impl_names[impl]);
  printf("Part: 0x%lx (%s)\n", partnum, part_name);
  printf("Variant: %ld\n", variant);
  printf("Revision: %ld\n", rev);

  char** cpu_outs[4];
  for (int i = 0; i < 4; i++) {
    cpu_outs[i] = ALLOC_MANY(char*, 100);

    for (int j = 0; j < 100; j++) {
      cpu_outs[i][j] = ALLOC_MANY(char, 1024);
      cpu_outs[i][j][0] = '\0';
    }
  }

  ENABLE_PGTABLE = 0; /* pgtable may not be setup yet, so dont try use it on the other booted CPUs */
  ensure_cpus_on();
  for (int cpu = 0; cpu < NO_CPUS; cpu++)
    run_on_cpu(cpu, __print_id_cpu, &cpu_outs[cpu][0]);

  /* each line from each CPU is of the form "ID_REG_NAME: 0x000ABC" */

  /* first calculate the maximum length of the ID lines */
  int maxlinelen = 0;
  for (int i = 0; i < 100; i++) {
    for (int cpu = 0; cpu < 4; cpu++) {
      maxlinelen = MAX(maxlinelen, strlen(cpu_outs[cpu][i]));
    }
  }

  /* we want to print a header
   * so first we work out the length of the indent for the title
   *      indent
   * vvvvvvvvvvvvvvvvvvvv
   *                      ID REGISTERS PER CPU                    <- title
   *            CPU0          CPU1            CPU2          CPU3  <- headers
   */
  int indent_width = ((maxlinelen+2)*4 - 22) / 2;
  for (int i = 0; i < indent_width; i++)
    printf(" ");
  printf(" ID REGISTERS PER CPU\n");

  /* now the headers
   * each right-aligned with the columns of the table
   *        CPU0        CPU1    ...       <- headers
   *    ID0: 0x0    ID0: 0x2    ...
   *    ID1: 0x1    ID1: 0x3    ...
   */
  for (int cpu = 0; cpu < 4; cpu++) {
    for (int i = 0; i < 2+maxlinelen-4; i++)
      printf(" ");

    printf("CPU%d", cpu);
  }
  printf("\n");

  /* and finally print each line, right-aligned in columns
   * underneath each header
   */
  for (int i = 0; i < 100; i++) {
    for (int cpu = 0; cpu < 4; cpu++) {
      char* line = cpu_outs[cpu][i];

      /* empty string means we ran off the end
       * and are at the end, so just abort
       * no need to cleanup */
      if (strcmp(line, ""))
        abort();

      int len = strlen(line);
      for (int j = 0; j < 2+maxlinelen-len; j++)
        printf(" ");
      printf("%s", line);
    }
    printf("\n");
  }

  abort();
}

static void n(char* x) {
  int Xn = atoi(x);
  NUMBER_OF_RUNS = Xn;
}

static void b(char* x) {
  int Xn = atoi(x);
  RUNS_IN_BATCH = Xn;
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

/* this init_cfg_state function gets called
 * after reading the args to do any housekeeping and cleanup
 *
 * e.g. checking mutual exclusive options and the like
 * that the argparser itself does not do
 */
void init_cfg_state(void) {
  /* ensure we use the correct runner for the given concretization algorithm */
  switch (LITMUS_CONCRETIZATION_TYPE) {
    case CONCRETE_RANDOM:
      LITMUS_RUNNER_TYPE = RUNNER_EPHEMERAL;
      break;
    case CONCRETE_LINEAR:
      LITMUS_RUNNER_TYPE = RUNNER_SEMI_ARRAY;
      break;
    case CONCRETE_FIXED:
      LITMUS_RUNNER_TYPE = RUNNER_EPHEMERAL;
      break;
  }
}

argdef_t COMMON_ARGS = (argdef_t){
  .args=(const argdef_arg_t*[]){
    OPT(
      "-h",
      "--help",
      help,
      "display this help text and quit\n"
      "\n"
      "display detailed help for OPTION.",
      .show_help_both=1,
      .arg=OPT_ARG_OPTIONAL,
      .metavar="OPTION",
    ),
    FLAG_ACTION(
      "-V",
      "--version",
      version,
      "display version information and quit\n"
      "\n"
      "displays full version info\n",
      .no_negation=true,
    ),
    FLAG_ACTION(
      NULL,
      "--id",
      device_ident_and_quit,
      "display device identification information and quit\n"
      "\n"
      "shows the information for the current device, and all ID registers.",
      .no_negation=true,
    ),
    FLAG(
      "-p",
      "--pgtable",
      ENABLE_PGTABLE,
      "enable/disable pagetable\n"
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
      .arg=OPT_ARG_NONE,
    ),
    OPT(
      "-s",
      "--seed",
      s,
      "initial seed\n"
      "\n"
      "at the beginning of each test, a random seed is selected.\n"
      "this seed can be forced with --seed=12345 which ensures some level of determinism.",
      .arg=OPT_ARG_REQUIRED,
    ),
    NULL,
  }
};


argdef_t LITMUS_ARGS = (argdef_t){
  .exe_name="litmus",
  .short_usage="[OPTION]... [TEST]...",
  .description="Run EL1/EL0 litmus tests",
  .args=(const argdef_arg_t*[]){
    OPT(
      NULL,
      "--show",
      show,
      "show list of tests and quit\n"
      "\n"
      "displays the complete list of the compiled tests and their groups, then quits.",
      .arg=OPT_ARG_NONE,
    ),
    FLAG(
      NULL,
      "--run-forever",
      RUN_FOREVER,
      "repeat test runs indefinitely\n"
    ),
    FLAG(
      NULL,
      "--perf",
      ENABLE_PERF_COUNTS,
      "enable/disable performance tests\n"
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
    OPT(
      "-b",
      "--batch-size",
      b,
      "number of runs per batch\n"
      "\n"
      "sets the number of runs per batch\n"
      "X must be an integer (default: 1).\n"
      "up to maximum number of ASIDs (e.g. 2^8).\n"
      "If not using --tlbsync=ASID then this must be 1"
    ),
    ENUMERATE(
      "--format",
      OUTPUT_FORMAT,
      output_style_t,
      2,
      ARR((const char*[]){"herdtools", "original"}),
      ARR((shuffle_type_t[]){STYLE_HERDTOOLS, STYLE_ORIGINAL}),
      "format to print output\n"
      "\n"
      "controls the format of the output\n"
      "\n"
      "herdtools: output compatible with herdtools7 suite\n"
      "original: original style of output"
    ),
    FLAG(
      NULL,
      "--hist",
      ENABLE_RESULTS_HIST,
      "enable/disable results histogram collection\n"
    ),
    FLAG(
      NULL,
      "--print-outcome-breakdown",
      ENABLE_RESULTS_OUTREG_PRINT,
      "prints the breakdown of observed outcomes (default: on)\n"
    ),
    FLAG(
      NULL,
      "--print-missing-warning",
      ENABLE_RESULTS_MISSING_SC_WARNING,
      "prints a warning if an expected outcome is not observed (default: on)\n"
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
      "asid: assign each test run an ASID and run in batches"
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
      " in-between tests."
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
      "rand: access pages in random order"
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
      "fixed: always use the same address, random or can be manually picked via --config-concretize"
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
      "  places x at 0x1234, y at 0x5678 and z at 0x9abc."
    ),
    NULL
  }
};

argdef_t UNITTEST_ARGS = (argdef_t){
  .exe_name="unittests",
  .short_usage="[OPTION]...",
  .description="Run system-litmus-harness unittests",
  .args=(const argdef_arg_t*[]){
    FLAG(
      NULL,
      "--test-linear-concretization",
      ENABLE_UNITTESTS_CONCRETIZATION_TEST_LINEAR,
      "include linear concretization tests (default: on)\n"
    ),
    FLAG(
      NULL,
      "--test-random-concretization",
      ENABLE_UNITTESTS_CONCRETIZATION_TEST_RANDOM,
      "include random concretization tests (default: on)\n"
    ),
    NULL,
  }
};

void read_args(int argc, char** argv) {
  argparse_read_args(argc, argv);
}