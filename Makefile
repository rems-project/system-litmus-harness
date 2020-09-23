define USAGE
Simple Usage:
   make	build		builds qemu and kvm targets
   make docker		builds docker container and runs the unittests
   make clean		remove built files in bin/
   make deepclean	clean everything. No really.
   make publish		publish doc/ folder to gh-pages
   make hw-results	collect hardware results from known sources
endef

define ADV_USAGE
Advanced Usage:
   make debug-litmus GDB="gdb-exe"
   	Runs qemu_litmus.exe in the background and attaches gdb
   make debug-unittests GDB="gdb-exe"
   	Runs qemu_unittests.exe in the background and attaches gdb
   make build-unittests
   	Builds the unittests and generates qemu_unittests.exe
   make lint
   	Runs automated linter against all litmus test C files and quits
   	See LINTER option below
   make collect-litmus LITMUS_TESTS="litmus-test-list"
   	Runs $$(MAKE_TEST_LIST_CMD) to build groups.c
   	See LITMUS_TESTS and MAKE_TEST_LIST_CMD options below
   make cleanlibs
   	remove built harness objects but leave compiled tests alone
   make cleantests
   	remove auto-generated test files in litmus/
   make docker-interact
   	start the docker container but attach a tty.
   make docker-litmus BIN_ARGS="args"
   	start the docker container but run the litmus tests, passing
   	BIN_ARGS to the qemu_litmus executable.
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
   make [...] TESTS="test-regexp"
   	Only run unittests whose names match test-regexp.
   make [...] LINTER="linter-exe"
   	Use linter-exe as the linter.
   	The Makefile will call `linter-exe file.c` for each
   	litmus test file after compiling it and pipe its output
   	to the user.
   	This option is disabled if ran with the -q flag
   make [...] MAKE_TEST_LIST_CMD="cmd-exe"
   	Use cmd-exe to build groups.c from given litmus test files.
   	This option is disabled if TEST_DISCOVER=0
   make collect-litmus [...] LITMUS_TESTS="litmus-tests-list"
   	Only compile tests in litmus-tests-list
   	Whitespace separated list of groups or test names.
   	Can use - to negate match.
   	example: TESTS="@all -@data"
   	This option is disabled if TEST_DISCOVER=0
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
	rm -f qemu_litmus
	rm -f qemu_unittests
	rm -f kvm_litmus
	rm -f kvm_unittests

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

# always re-build *exe.exe
# this ensures the changes to the QEMU flags
# are seen by the rebuild
.PHONY: FORCE
FORCE:
bin/qemu_%.exe: FORCE

include mk/deepclean.mk
include mk/docs.mk
include mk/qemu.mk
include mk/build.mk
include mk/docker.mk
include mk/litmus.mk
include mk/unittests.mk

.PHONY: qemu
qemu: qemu_litmus qemu_unittests

.PHONY: kmv
kvm: kvm_litmus kvm_unittests

.PHONY: build
build: qemu kvm