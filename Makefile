define USAGE
Simple Usage:
   make	build		builds qemu and kvm targets
   make docker		builds docker container and runs the unittests
   make clean		remove built files in bin/
   make deepclean	clean everything. No really.
   make publish		publish doc/ folder to gh-pages
   make hw-results	collect hardware results from known sources
   make test		run the unittests in QEMU
   make run			run the litmus tests in QEMU
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

NO_ROOT_CHECK ?= 0

ifeq ($(shell echo $${EUID}),0)
ifeq ($(NO_ROOT_CHECK),0)
  $(error You are running this makefile as root. \
          if you really intend this then pass NO_ROOT_CHECK=1. \
		  this makefile should not require root)
else ifeq ($(NO_ROOT_CHECK),1)
  $(warning detected root, but NO_ROOT_CHECK=1 so ignoring)
else
  $(error Unknown NO_ROOT_CHECK value '$(NO_ROOT_CHECK)' (expected 0 or 1))
endif
endif

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

# possible combinations of targets
PREFIXES = kvm qemu
BINTARGETS = litmus unittests

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
# bin/litmus.elf.S with line numbers if DEBUG=1
DEBUG = 0

DEBUG_FLAGS =
DEBUG_CFLAGS =
DEBUG_OBJDUMPFLAGS =

ifeq ($(DEBUG),1)
DEBUG_CFLAGS += -g -gdwarf-4 \
	-fno-omit-frame-pointer
DEBUG_CFLAGS += $(patsubst %,-DDEBUG_%=1,$(DEBUG_FLAGS))
DEBUG_OBJDUMPFLAGS += -g -l -r
else ifeq ($(DEBUG),0)
else
  $(error Unknown DEBUG value '$(DEBUG)' (expected 0 or 1))
endif

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
_HEAD_STATUS = $(shell git status -s)
_HEAD_COMMIT_HASH = $(_HEAD_COMMIT_SHA)$(if $(_HEAD_STATUS),!,)
_DATE_VERSION = $(shell date '+%y.%m')
_MINOR_VERSION = $(shell git log HEAD --since=`date '+%Y-%m'`-01 --pretty=format:%ci | wc -l)
_VERSION = $(_DATE_VERSION).$(_MINOR_VERSION)
CFLAGS = -O0 -nostdlib \
		$(foreach DIR,$(INC_DIRS),-I $(DIR)) \
		$(foreach DIR,$(OTHER_INCLUDES),-I $(DIR)) \
		-ffreestanding -fomit-frame-pointer -fno-pie -fno-pic \
		-Wall $(addprefix -Wno-,$(CCNOWARN)) $(addprefix -Werror=,$(CCERRORS)) \
		-Wshadow \
		-D__VERSION_STR__="\"$(_VERSION)\"" \
		-DCOMMITHASH="\"$(_HEAD_COMMIT_HASH)\"" \
		$(DEBUG_CFLAGS)

LDFLAGS = -nostdlib -n -static -pie
OBJDUMPFLAGS = $(DEBUG_OBJDUMPFLAGS)
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

include mk/deepclean.mk
include mk/docs.mk
include mk/qemu.mk
include mk/build.mk
include mk/docker.mk

define INSTALL
$(call run_cmd,INSTALL,./$(2),\
	cp $(1) $(2))
endef

define CLEAN
$(call run_cmd,CLEAN,./$(2),\
	rm $(1) $(2))
endef

.PHONY: clean
clean:
	$(call CLEAN,-rf,bin/)

ifeq ($(strip $(TEST_DISCOVER)),1)
	$(call CLEAN,-f,litmus/groups.c)
endif
ifndef TARGET_DEEPCLEAN
	@echo 'run `make cleantests` to remove test and group lists too'
endif

.PHONY: cleanlibs
cleanlibs:
	$(call CLEAN,-f,bin/*.o)
	$(call CLEAN,-f,bin/litmus.*)
	$(call CLEAN,-rf,bin/lib/)
ifndef TARGET_DEEPCLEAN
	@echo 'run `make clean` to remove compiled tests too'
endif

.PHONY: cleantests
cleantests:
	$(call CLEAN,-f,litmus/test_list.txt)
	$(call CLEAN,-f,litmus/group_list.txt)
	$(call CLEAN,-f,litmus/linter.log)

# list of PHONY targets that will need
# files in the litmus/litmus_tests/ dir
# for auto-discovery.
#
# this gets added to as we add new targets
LITMUS_TARGETS =

# per device targets
# if we have target build-litmus
# then we generate the build-litmus-rpi3 and build-litmus-rpi4 etc targets
# which run build-litmus with the configuration for that device
# and then copies the top-level exes over
# so we end up with ./rpi3_qemu_litmus and ./rpi4_qemu_litmus etc
# rather than the ./qemu_litmus executable

# $(call stem,...) turns "-x" into "x"
# and "x" into "x"
# and "" into ""
stem = $(if $(1),$(patsubst -%,%,$(strip $(1))),default)

# exe_prefix turns "-x" into "x_"
# "x" into "x"
# and "" into ""
exe_prefix = $(patsubst -%,%_,$(strip $(1)))

# auto-generate targets for the different variations of the build
define build-target

# this inserts spurious whitespace but since it's a single
# space char per loop it does not matter as make will ignore it
 $(foreach d,$(strip $(2)),\
 $(foreach p,$(PREFIXES),\
 $(foreach t,$(BINTARGETS),\
bin/$(call exe_prefix,$(1))$(p)_$(t).exe: $(d)
)))


# build separate bin/ exes for each
# so we can make sure they get the right options
$(foreach t,$(BINTARGETS),\
bin/$(call exe_prefix,$(1))qemu_$(t).exe: OUT_NAME=$$$$tmp
bin/$(call exe_prefix,$(1))qemu_$(t).exe: bin/$(t).bin
	$$(call run_cmd,BUILD_EXE,$$@, \
		$$(call make_exe,$$(RUN_CMD_LOCAL) $$$${QEMU_ARGS}) \
	)
)

$(foreach t,$(BINTARGETS),\
bin/$(call exe_prefix,$(1))kvm_$(t).exe: OUT_NAME=$$$$tmp
bin/$(call exe_prefix,$(1))kvm_$(t).exe: bin/$(t).bin
	$$(call run_cmd,BUILD_EXE,$$@,\
		$$(call make_exe,$$(RUN_CMD_HOST) $$$${QEMU_ARGS}) \
	)
)

# generate all the build targets
# e.g. build-kvm-litmus-rpi4
#
# we patsubst to remove the leading - if it exists and put a underscore suffix
# this ensures that $ (call build-target,,,) generates plain ./$(p)_$(t) targets
# whereas a $ (call build-target,-prefix,) generates ./prefix_$(p)_$(t)
#
# p.s. ignore the space between $ and call, if it wasn't there then this comment
# in this define would be expanded and the makefile wouldn't terminate
$(foreach p,$(PREFIXES),\
$(foreach t,$(BINTARGETS),\
./$(call exe_prefix,$(1))$(p)_$(t): bin/$(call exe_prefix,$(1))$(p)_$(t).exe
	$$(call INSTALL,bin/$(call exe_prefix,$(1))$(p)_$(t).exe,$(call exe_prefix,$(1))$(p)_$(t))
)
)

$(foreach p,$(PREFIXES),\
$(foreach t,$(BINTARGETS),\
.PHONY: build-$(p)-$(t)-$(call stem,$(1))
build-$(p)-$(t)-$(call stem,$(1)): ./$(call exe_prefix,$(1))$(p)_$(t)
)
)

# now generate all the build-PREFIX-DEVICE targets
# e.g. build-kvm-rpi3
$(foreach p,$(PREFIXES),\
.PHONY: build-$(p)-$(call stem,$(1))
build-$(p)-$(call stem,$(1)): $(foreach t,$(BINTARGETS),bin/$(call exe_prefix,$(1))$(p)_$(t).exe) $(foreach t,$(BINTARGETS),build-$(p)-$(t)-$(call stem,$(1)))
LITMUS_TARGETS += $(call exe_prefix,$(1))$(p)_litmus
LITMUS_TARGETS += build-$(p)-litmus-$(call stem,$(1))
LITMUS_TARGETS += build-$(p)-$(call stem,$(1))
)

# and the build-TARGET-DEVICE targets
# e.g. build-litmus-graviton2
$(foreach t,$(BINTARGETS),\
.PHONY: build-$(t)-$(call stem,$(1))
build-$(t)-$(call stem,$(1)): $(foreach p,$(PREFIXES),bin/$(call exe_prefix,$(1))$(p)_$(t).exe) $(foreach p,$(PREFIXES),build-$(p)-$(t)-$(call stem,$(1)))
$(if $(filter litmus,$(t)),\
LITMUS_TARGETS += build-$(t)-$(call stem,$(1))
)
)

# finally put them together into a build-DEVICE target
# e.g. build-rpi3
.PHONY: build-$(call stem,$(1))
build-$(call stem,$(1)): $(foreach t,$(BINTARGETS),$(foreach p,$(PREFIXES),bin/$(call exe_prefix,$(1))$(p)_$(t).exe)) $(foreach t,$(BINTARGETS),$(foreach p,$(PREFIXES),build-$(p)-$(t)-$(call stem,$(1))))
LITMUS_TARGETS += build-$(call stem,$(1))

# add a cleanup target
# n.b. No need to remove bin/*exe files
# as clean target already clears all of bin/
.PHONY: clean-$(call stem,$(1))
clean-$(call stem,$(1)):
	$$(call run_cmd,CLEAN,. ($(call stem,$(1)) binaries), \
		{	$(foreach p,$(PREFIXES), \
				$(foreach t,$(BINTARGETS), \
					rm -f $(call exe_prefix,$(1))$(p)_$(t) ; \
				) \
			) \
		} \
	)

# add our new cleanup target as a prerequesite to the basic 'clean'
# so `make clean` also hits these targets
clean: | clean-$(call stem,$(1))
endef

# populate the targets
$(eval $(call build-target,,))
$(eval $(call build-target,-rpi3,HOST=no-gic QEMU_MEM=512M))
$(eval $(call build-target,-rpi4,HOST=gic QEMU_MEM=1G))
$(eval $(call build-target,-graviton2,HOST=gic QEMU_MEM=1G))

.PHONY: build
build: build-default
LITMUS_TARGETS += build

test: ./qemu_unittests
test:
	./qemu_unittests $(BIN_ARGS)

run: ./qemu_litmus
	./qemu_litmus $(BIN_ARGS)
LITMUS_TARGETS += run

include mk/litmus.mk
include mk/unittests.mk

# provide a list of all the potential build- targets
.PHONY: list
list:
	@echo 'Please choose from the following targets:'
	@$(MAKE) -prRq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | grep -o '^build[^:]*' | grep -o '^[^$$]*$$' | sort