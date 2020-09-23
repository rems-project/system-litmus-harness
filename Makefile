define USAGE
Simple Usage:
   make	build		builds bin/qemu_litmus.exe and bin/kvm_litmus.exe
   make run 		runs `make qemu` then runs all tests
   make clean		remove built files in bin/
   make deepclean	clean everything. No really.
   make publish		publish doc/ folder to gh-pages
   make hw-results	collect hardware results from known sources
endef

define ADV_USAGE
Advanced Usage:
   make run -- args args args
   	run local qemu with args
   	e.g. make run -- MP+pos -n500 --pgtable
   make debug GDB="gdb-exe"
   	Runs `make run` in the background and attaches gdb
   make ssh SSH_NAME="ssh-name" BIN_ARGS="bin-args"
   	Runs `make kvm` and then scp's the kvm_litmus.exe file over
   	to ssh-name where it is ran with argv bin-args.
   make unittests
   	Builds the unittests and runs them in QEMU
   make lint
   	Runs automated linter against all litmus test C files and quits
   	See LINTER option below
   make collect_tests TESTS="litmus-test-list"
   	Runs $$(MAKE_TEST_LIST_CMD) to build groups.c
   	See TESTS and MAKE_TEST_LIST_CMD options below
   make cleanlibs
   	remove built harness objects but leave compiled tests alone
   make cleantests
   	remove auto-generated test files in litmus/
Options:
   make [...] -q
   	Show minimal/no output
   make [...] VERBOSE=1
   	Show full commands and output
   make [...] PREFIX="prefix"
   	Use $$(PREFIX)gcc and $$(PREFIX)ld during build
   make [...] NO_CHECK=1
    Do not perform existence checks of cross-compilation tools.
   make kvm [...] HOST="no-gic"
   	Use KVM with virtualized interrupt controller
   make [...] SHOW_PREPROCESSED_OUTPUT=1
   	For each .o file generate .pp containing pre-processor output
   make [...] TEST_DISCOVER=0
   	Disable litmus test discovery.
   	This disables generation of groups.c, so the user must manage
   	groups.c manually with this option.
   make unittests [...] TESTS="test-regexp"
   	Only run test cases matching test-regexp.
   make [...] LINTER="linter-exe"
   	Use linter-exe as the linter.
   	The Makefile will call `linter-exe file.c` for each
   	litmus test file after compiling it and pipe its output
   	to the user.
   	This option is disabled if ran with the -q flag
   make [...] MAKE_TEST_LIST_CMD="cmd-exe"
   	Use cmd-exe to build groups.c from given litmus test files.
   	This option is disabled if TEST_DISCOVER=0
   make collect_tets [...] TESTS="litmus-tests-list"
   	Only compile tests in litmus-tests-list
   	Whitespace separated list of groups or test names.
   	Can use - to negate match.
   	example: TESTS="@all -@data"
   	This option is disabled if TEST_DISCOVER=0
   	Note: meaning of $$(TESTS) depends on whether target is unittests or litmus
endef

.PHONY: shorthelp
shorthelp:
	$(info $(USAGE))
	@echo 'Run `make help` for more info'

.PHONY: help
help:
	$(info $(USAGE))
	$(info )
	$(info $(ADV_USAGE))
	@: # do nothing and do not echo

# Compiler and tools options
KNOWN_PREFIXES = aarch64-linux-gnu- aarch64-none-elf-
PREFIX = $(word 1, $(foreach PRF,$(KNOWN_PREFIXES),$(if $(shell which $(PRF)gcc),$(PRF),)))
CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
QEMU = qemu-system-aarch64
GDB = $(PREFIX)gdb

# set to 1 to do a second CC pass with -E
# this will generate not only bin/**/f.o files but bin/**/f.pp files that are the output
# of the preprocessor.
SHOW_PREPROCESSED_OUTPUT = 0

# will build with debugging symbols and dump
# bin/litmus.elf.S if DEBUG=1
DEBUG = 0
# TODO: finish this

DEBUG_FLAGS =
DEBUG_CFLAGS = $(patsubst %,-DDEBUG_%=1,$(DEBUG_FLAGS))

# if NO_CHECK=1 then do not check for existence of the above
# cross-compilation tools on the local machine
NO_CHECK = 0

ifeq ($(NO_CHECK),0)
else ifeq ($(NO_CHECK),1)
else
  $(info $(USAGE))
  $(error Unexpected NO_CHECK="$(NO_CHECK)" param)
endif

# if running with -s (silent mode)
ifneq ($(findstring s,$(filter-out --%,$(MAKEFLAGS))),)
  quiet = 1
else
  quiet = 0
endif

# set VERBOSE=1 to disable pretty-out
VERBOSE = 0

SSH_NAME = pi@rems-rpi4b

CCERRORS = return-type parentheses misleading-indentation null-dereference sequence-point uninitialized maybe-uninitialized
CCNOWARN =
INC_DIRS = inc inc/litmus inc/vmm inc/re
LIB_DIRS = lib lib/arch lib/arch/vmm lib/valloc lib/re lib/drivers/qemu lib/litmus_test lib/litmus_test/concretization
OTHER_INCLUDES =  # set for unittests
_HEAD_COMMIT_SHA = $(shell git rev-parse --short HEAD -n1)
_DATE_VERSION = $(shell date '+%y.%m')
CFLAGS = -O0 -nostdlib \
		$(if $(DEBUG),-g -gdwarf-4,) \
		$(foreach DIR,$(INC_DIRS),-I $(DIR)) \
		$(foreach DIR,$(OTHER_INCLUDES),-I $(DIR)) \
		-ffreestanding -fomit-frame-pointer -fno-pie -fno-pic \
		-Wall $(addprefix -Wno-,$(CCNOWARN)) $(addprefix -Werror=,$(CCERRORS)) \
		-D__VERSION_STR__="\"$(_DATE_VERSION)\"" \
		-DCOMMITHASH="\"$(_HEAD_COMMIT_SHA)\"" \
		$(DEBUG_CFLAGS)

LDFLAGS = -nostdlib -n -static -pie
OBJDUMPFLAGS = $(if $(DEBUG),-g -l -r,)
SSHFLAGS =

LITMUS_TESTS = # which litmus tests to collect
# if TEST_DISCOVER is 1 then the makefile will run $(MAKE_TEST_LIST_CMD)
# to generate groups.c
# if TEST_DISCOVER is 0 then the user must supply their own groups.c
# and must also supply LITMUS_FILES, a list of all the .c litmus test files
TEST_DISCOVER = 1
MAKE_TEST_LIST_CMD = python3 litmus/makegroups.py

LINTER = python3 litmus/linter.py
NO_LINT = 0

ifneq ($(findstring help,$(filter-out --%,$(MAKECMDGOALS))),)
   TARGET_HELP = 1
endif

ifneq ($(findstring deepclean,$(MAKECMDGOALS)),)
   TARGET_DEEPCLEAN = 1
   ifneq ($(MAKECMDGOALS),deepclean)
      $(error deepclean target must be the only target)
   endif
endif

.PHONY: clean
clean:
	rm -rf bin/
ifeq ($(strip $(TEST_DISCOVER)),1)
	rm -f litmus/groups.c
endif
ifndef TARGET_DEEPCLEAN
	@echo 'run `make cleantests` to remove test and group lists too'
endif

.PHONY: cleanlibs
cleanlibs:
	rm -f bin/*.o
	rm -f bin/litmus.*
	rm -rf bin/lib/
ifndef TARGET_DEEPCLEAN
	@echo 'run `make clean` to remove compiled tests too'
endif

.PHONY: cleantests
cleantests:
	rm -f litmus/test_list.txt
	rm -f litmus/group_list.txt
	rm -f litmus/linter.log

# always re-build *_litmus.exe
# this ensures the changes to the QEMU flags
# are seen by the rebuild
.PHONY: FORCE
FORCE:
bin/kvm_litmus.exe: FORCE
bin/qemu_litmus.exe: FORCE

# determine whether we have a target that requires building litmus
ifneq ($(findstring build,$(MAKECMDGOALS)),)
else ifneq ($(findstring ssh,$(MAKECMDGOALS)),)
else ifneq ($(findstring run,$(MAKECMDGOALS)),)
else ifneq ($(findstring lint,$(MAKECMDGOALS)),)
else ifneq ($(findstring debug,$(MAKECMDGOALS)),)
else
  no_run_litmus = 1
endif

include mk/deepclean.mk
include mk/docs.mk
include mk/qemu.mk
include mk/build.mk
include mk/docker.mk

ifndef no_run_litmus
  include mk/litmus.mk
endif

ifneq ($(findstring unittests,$(MAKECMDGOALS)),)
  ifndef no_run_litmus
    $(error Cannot build both unittests and litmus binaries at the same time)
  endif
  include mk/unittests.mk
endif