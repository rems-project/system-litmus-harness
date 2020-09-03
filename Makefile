define USAGE
Simple Usage:
   make			equivalent to `make all`
   make all		equivalent to `make kvm qemu`
   make qemu		builds bin/qemu_litmus.exe which runs QEMU
   make kvm		builds bin/kvm_litmus.exe which uses KVM
   make run 		runs `make qemu` then runs all tests
   make clean		remove built files in bin/
   make cleanlibs	remove built harness objects but leave compiled tests alone
   make cleantests	remove auto-generated test files in litmus/
   make publish		publish doc/ folder to gh-pages
   make hw-results	collect hardware results from known sources

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
   make litmus_tests LITMUS_TESTS="litmus-test-list"
   	Runs $$(MAKE_TEST_LIST_CMD) to build groups.c
   	See LITMUS_TESTS and MAKE_TEST_LIST_CMD options below

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
   make [...] LITMUS_TESTS="litmus-tests-list"
   	Only compile tests in litmus-tests-list
   	Whitespace separated list of groups or test names.
   	Can use - to negate match.
   	example: LITMUS_TESTS="@all -@data"
   	This option is disabled if TEST_DISCOVER=0
endef

# for test discovery
LITMUS_TESTS = # which litmus tests to collect
TEST_DISCOVER = 1 # set to 0 to disable auto-test discovery
# if auto test discovery is disabled, then the author must manage groups.c manually.
MAKE_TEST_LIST_CMD = python3 litmus/makegroups.py

# Compiler and tools options
KNOWN_PREFIXES = aarch64-linux-gnu- aarch64-none-elf-
PREFIX = $(word 1, $(foreach PRF,$(KNOWN_PREFIXES),$(if $(shell which $(PRF)gcc),$(PRF),)))

CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
QEMU = qemu-system-aarch64
GDB = gdb-multiarch  --eval-command "set arch aarch64"

# if NO_CHECK=1 then do not check for existence of the above
# cross-compilation tools on the local machine
NO_CHECK = 0

ifeq ($(NO_CHECK),0)
else ifeq ($(NO_CHECK),1)
else
  $(info $(USAGE))
  $(error Unexpected NO_CHECK="$(NO_CHECK)" param)
endif

ifeq ($(word 1,$(MAKECMDGOALS)),run)
  define leading
    $(if $(findstring run,$(firstword $1)),sep $(firstword $1),\
      $(firstword $1) $(call leading, $(wordlist 2,$(words $1),$1)))
  endef
  # assume format of command is `make [opt...] run -- cmds`
  BIN_ARGS = $(wordlist $(words $(call leading,$(MAKECMDGOALS))),$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # make dummy targets for BIN_ARGS so Make doesn't try run them
  # see https://stackoverflow.com/questions/2214575/passing-arguments-to-make-run
  $(eval $(BIN_ARGS):;@:)
else
  BIN_ARGS =
endif

HOST = gic

# set to 1 to do a second CC pass with -E
# this will generate not only bin/**/f.o files but bin/**/f.pp files that are the output
# of the preprocessor.
SHOW_PREPROCESSED_OUTPUT = 0

# if running with -s (silent mode)
ifneq ($(findstring s,$(filter-out --%,$(MAKEFLAGS))),)
  quiet = 1
else
  quiet = 0
endif

# set VERBOSE=1 to disable pretty-out
VERBOSE = 0

OUT_NAME = bin/litmus.bin
SSH_NAME = pi@rems-rpi4b

# unittest list
TESTS = .

RUN_CMD_HOST_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt,gic-version=host --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"
RUN_CMD_HOST_NO_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

ifeq ($(HOST),gic)
  RUN_CMD_HOST = $(RUN_CMD_HOST_GIC)
else ifeq ($(HOST),no-gic)
  RUN_CMD_HOST = $(RUN_CMD_HOST_NO_GIC)
else
  $(info $(USAGE))
  $(error Unexpected HOST="$(HOST)" param)
endif

RUN_CMD_LOCAL = 	\
	$(QEMU) \
		-nodefaults -machine virt -cpu cortex-a57 \
		-device virtio-serial-device \
		-display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

CCERRORS = return-type parentheses misleading-indentation null-dereference sequence-point uninitialized maybe-uninitialized
CCNOWARN =
INC_DIRS = inc inc/litmus inc/vmm inc/re
LIB_DIRS = lib lib/arch lib/arch/vmm lib/valloc lib/re lib/drivers/qemu lib/litmus_test lib/litmus_test/concretization
OTHER_INCLUDES =  # set for unittests
_HEAD_COMMIT_SHA = $(shell git rev-parse --short HEAD -n1)
_DATE_VERSION = $(shell date '+%y.%m')
CFLAGS = -O0 -nostdlib \
		-g -gdwarf-4 \
		$(foreach DIR,$(INC_DIRS),-I $(DIR)) \
		$(foreach DIR,$(OTHER_INCLUDES),-I $(DIR)) \
		-ffreestanding -fomit-frame-pointer -fno-pie -fno-pic \
		-Wall $(addprefix -Wno-,$(CCNOWARN)) $(addprefix -Werror=,$(CCERRORS)) \
		-D__VERSION_STR__="\"$(_DATE_VERSION)\"" \
		-DCOMMITHASH="\"$(_HEAD_COMMIT_SHA)\""

LDFLAGS = -nostdlib -n -static -pie
OBJDUMPFLAGS = -g -l -r
SSHFLAGS =

ifeq ($(findstring clean,$(MAKECMDGOALS)),clean)
  LITMUS_TARGET_CLEAN := 1
endif

# can pass LITMUS_FILES manually to supply a space-separated list of all the .c files to compile and link in the litmus/ directory
# otherwise the Makefile will try auto-discover the litmus/*.c files and the litmus/litmus_tests/ files
#	(according to either the supplied LITMUS_TESTS or checking against previous runs)
# or if TEST_DISCOVER=0  then just try compile *all* .c files inside litmus/ and litmus/litmus_tests/**/
# (but the user has to manually edit groups.c so the harness itself knows about them)
ifeq ($(strip $(TEST_DISCOVER)),1)
   ifndef LITMUS_FILES
   ifndef LITMUS_TARGET_CLEAN
      # if fail, ignore
      out := $(shell ($(MAKE_TEST_LIST_CMD) $(quiet) $(LITMUS_TESTS) 2>/dev/null) || echo "FAIL")

      ifeq ($(out),FAIL)
        $(warning \
			Generation of litmus/groups.c failed. \
		 	Expecting user management of groups.c file. \
		)

        outcp := $(shell [ -f litmus/groups.c ] || (echo FAIL; cp litmus/backup/* litmus/))

        ifeq ($(outcp),FAIL)
            $(warning \
				No groups.c file detected. \
		 		Creating fresh ... \
			)
        endif
      endif

   	  litmus_test_list = $(shell awk '$$1==1 {print $$2}' litmus/test_list.txt)
   	  LITMUS_TEST_FILES ?= $(litmus_test_list)
   endif
   endif
else
   LITMUS_TEST_FILES ?= $(wildcard litmus/litmus_tests/**/*.c)
endif

LITMUS_FILES := $(wildcard litmus/*.c) $(LITMUS_TEST_FILES)
LIB_FILES := $(foreach DIR,$(LIB_DIRS), $(wildcard $(DIR)/*.c))
UNITTESTS_FILES := $(wildcard unittests/*.c) $(wildcard unittests/testlib/*.c) $(wildcard unittests/tests/**/*.c)

TOP_ASM_FILES := $(wildcard *.S)
TOP_C_FILES := $(wildcard *.c)

ASM_FILES := $(TOP_ASM_FILES)
C_FILES := $(LIB_FILES) $(TOP_C_FILES)
COMMON_BIN_FILES := $(addprefix bin/,$(C_FILES:.c=.o) $(ASM_FILES:.S=.o))
litmus_BIN_FILES := $(addprefix bin/,$(LITMUS_FILES:.c=.o))
unittests_BIN_FILES := $(addprefix bin/,$(UNITTESTS_FILES:.c=.o))

LINTER = python3 litmus/linter.py
NO_LINT = 0

# defines a helper run_cmd
# this helper displays nice text and handles VERBOSE/quiet modes gracefully
# Usage: $(call run_cmd,SHORTHAND,ARGNAME,SHCOMMAND)
ifeq ($(VERBOSE),1)
define run_cmd
	$3
endef
else ifeq ($(quiet),1)
define run_cmd
	@$3
endef
else
define run_cmd
	@echo "   ($1) $2" 2>&1
	@$3
endef
endif

# runs $(CC) on the input
# the input source file must be the first pre-requesite in the rule
ifeq ($(SHOW_PREPROCESSED_OUTPUT),1)
  define run_cc
	@mkdir -p $(dir $@)
	$(call run_cmd,CC,$<,\
		$(CC) $(CFLAGS) -c -o $@ $< \
		; $(CC) $(CFLAGS) -E -o $(patsubst %.o,%.pp,$@) $<  \
	)
  endef
else
  define run_cc
	@mkdir -p $(dir $@)
	$(call run_cmd,CC,$<,\
		$(CC) $(CFLAGS) -c -o $@ $< \
	)
  endef
endif

define check_tool
	$(call run_cmd,CHECK,$1,\
		if ! which "$2" 1>/dev/null; then \
			echo error: Command "'$2'" not found!; \
			exit 1; \
		fi \
	)
endef

.PHONY: all
all: kvm qemu

.PHONY: kvm
kvm: bin/kvm_litmus.exe

.PHONY: qemu
qemu: bin/qemu_litmus.exe

# checks that the specified cross-compilation tools actually exist on the machine
.PHONY: check_cross_tools_exist
check_cross_tools_exist:
ifeq ($(NO_CHECK),0)
	$(call check_tool,CC,$(CC))
	$(call check_tool,LD,$(LD))
	$(call check_tool,OBJCOPY,$(OBJCOPY))

ifeq ($(NO_LINT),0)
	$(call run_cmd,CHECK,LINTER,\
		if ! $(LINTER) "(find litmus/litmus_tests -name '*.c' | head -n1)" &>/dev/null; then \
			echo error: Linter not functional!; \
			echo Run again with NO_LINT=1 to disable linting; \
			exit 1; \
		fi \
	)
endif
endif

.PHONY: help
help:
	$(info $(USAGE))
	@: # do nothing and do not echo


.PHONY: lint
lint:
	@$(LINTER) $(LITMUS_TEST_FILES)

.PHONY: litmus_tests
litmus_tests:
	$(call run_cmd,make,Collecting new tests,\
		 $(MAKE_TEST_LIST_CMD) $(quiet) $(LITMUS_TESTS) || { \
			echo "Warning: failed to update groups.c. test listing may be out-of-sync." 1>&2 ; \
			( which python3 &> /dev/null ) && echo "litmus_tests target requires python3." 1>&2 ; \
		};\
	)
	@echo 're-run `make` to compile any new or updated litmus tests'

bin/lib/%.o: lib/%.c | check_cross_tools_exist
	$(run_cc)

bin/unittests/%.o: OTHER_INCLUDES=-I unittests/include
bin/unittests/%.o: unittests/%.c | check_cross_tools_exist
	$(run_cc)

bin/litmus/%.o: litmus/%.c | check_cross_tools_exist
	$(run_cc)
ifeq ($(quiet),0)
ifeq ($(NO_LINT),0)
	@$(LINTER) $<
endif
endif

# this is ugly, should use the same system as above ...
# so we don't have to re-build main.o each time.
.PHONY: unittests/main.c
bin/unittests/main.o: CFLAGS+=-DNO_TEST_FILES=$(strip $(shell ./unittests/getunittests.sh $(TESTS)))

bin/cpu_entry.o:  cpu_entry.S
	$(run_cc)

bin/vector_table.o:  vector_table.S
	$(run_cc)

bin/litmus.o: litmus_tests/main.c
	$(run_cc)

# remove bin/lib/version.o and let that build separately
# so any change to any of the other prerequesites will
# force a re-fresh of version.o containing the version string
ELF_PREREQ = $(filter-out bin/lib/version.o, $(COMMON_BIN_FILES)) $(litmus_BIN_FILES)
bin/lib/version.o: $(ELF_PREREQ)
# then add version.o back in so direct changes to it also cause a rebuild
# then make the ELF from all of them
bin/litmus.elf: bin/lib/version.o $(ELF_PREREQ)
	$(call run_cmd,LD,$@,\
		$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(litmus_BIN_FILES))
	$(call run_cmd,OBJDUMP,$@.S,\
		$(OBJDUMP) $(OBJDUMPFLAGS) -D $@ > $@.S)
	$(call run_cmd,CP,$@.debug,\
		cp $@ $@.debug)
	$(call run_cmd,STRIP,$@,\
		$(OBJCOPY) -g $@)

bin/unittests.elf: $(COMMON_BIN_FILES) $(unittests_BIN_FILES)
	$(call run_cmd,LD,$@,\
		$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(unittests_BIN_FILES))
	$(call run_cmd,OBJDUMP,$@.S,\
		$(OBJDUMP) $(OBJDUMPFLAGS) -D $@ > $@.S)

bin/litmus.bin: bin/litmus.elf
	$(call run_cmd,OBJCOPY,$@,\
		$(OBJCOPY) -O binary bin/litmus.elf bin/litmus.bin)

bin/unittests.bin: bin/unittests.elf
	$(call run_cmd,OBJCOPY,$@,\
		$(OBJCOPY) -O binary bin/unittests.elf bin/unittests.bin)

# Builds a standalone executable from the elf/bin file
# Usage: $(call make_exe,QEMU-COMMAND)
define make_exe =
	{	echo 'tmp=`mktemp`' > $@ ; \
		echo 'base64 -d << BIN_EOF | zcat > $$tmp || exit 2' >> $@ ; \
		gzip -c $< | base64 >> $@ ; \
		echo "BIN_EOF" >> $@ ; \
		echo '$1' >> $@ ; \
		chmod +x $@ ; \
	}
endef

bin/debug_%.exe: OUT_NAME=$$tmp
bin/debug_%.exe: bin/%.bin
	$(call run_cmd,BUILD_EXE,$@,\
		$(call make_exe,$(RUN_CMD_LOCAL) -S -s)\
	)

bin/qemu_%.exe: OUT_NAME=$$tmp
bin/qemu_%.exe: bin/%.bin
	$(call run_cmd,BUILD_EXE,$@,\
		$(call make_exe,$(RUN_CMD_LOCAL) $${QEMU_ARGS})\
	)

bin/kvm_%.exe: OUT_NAME=$$tmp
bin/kvm_%.exe: bin/%.bin
	$(call run_cmd,BUILD_EXE,$@,\
		$(call make_exe,$(RUN_CMD_HOST) $${QEMU_ARGS})\
	)

.PHONY: run
run: bin/qemu_litmus.exe
	./bin/qemu_litmus.exe $(BIN_ARGS)

debug: bin/debug_litmus.exe
	{ ./bin/debug_litmus.exe $(BIN_ARGS) & echo $$! > bin/.debug.pid; }
	$(GDB) --eval-command "target remote localhost:1234"
	{ cat bin/.debug.pid | xargs kill $$pid ; rm bin/.debug.pid; }

unittests: OUT_NAME=bin/unittests.bin
unittests: bin/qemu_unittests.exe
	./bin/qemu_unittests.exe $(BIN_ARGS)

ssh: bin/kvm_litmus.exe
	scp bin/kvm_litmus.exe $(SSH_NAME):litmus.exe
	ssh $(SSHFLAGS) $(SSH_NAME) "./litmus.exe '$(BIN_ARGS)'"

.PHONY: clean
clean:
	rm -rf bin/
ifeq ($(strip $(TEST_DISCOVER)),1)
	rm -f litmus/groups.c
endif
	@echo 'run `make cleantests` to remove test and group lists too'

.PHONY: cleanlibs
cleanlibs:
	rm -f bin/*.o
	rm -f bin/litmus.*
	rm -rf bin/lib/
	@echo 'run `make clean` to remove compiled tests too'

.PHONY: cleantests
cleantests:
	rm -f litmus/test_list.txt
	rm -f litmus/group_list.txt

# always re-build *_litmus.exe
# this ensures the changes to the QEMU flags
# are seen by the rebuild
.PHONY: FORCE
FORCE:
bin/kvm_litmus.exe: FORCE
bin/qemu_litmus.exe: FORCE

.PHONY: publish
publish:
	$(MAKE) -C doc publish

.PHONY: hw-results
hw-results:
	$(MAKE) -C hw-results EXCLUDES=checks,errata
