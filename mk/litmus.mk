# the top-level targets this file "exports"
.PHONY: lint
.PHONY: debug-litmus
.PHONY: collect-litmus
.PHONY: count-litmus-tests

LITMUS_TARGETS += lint collect-litmus debug-litmus count-litmus-tests

ifneq ($(strip $(foreach lt,$(LITMUS_TARGETS),$(foreach g,$(MAKECMDGOALS),$(filter $(lt),$(g))))),)
# or any target that mentions the litmus/ dir
else ifneq ($(findstring litmus/,$(MAKECMDGOALS)),)
else
  no_collect_tests = 1
endif

# can pass LITMUS_FILES manually to supply a space-separated list of all the .c files to compile and link in the litmus/ directory
# otherwise the Makefile will try auto-discover the litmus/*.c files and the litmus/litmus_tests/ files
#	(according to either the supplied LITMUS_TESTS or checking against previous runs)
# or if TEST_DISCOVER=0  then just try compile *all* .c files inside litmus/ and litmus/litmus_tests/**/
# (but the user has to manually edit groups.c so the harness itself knows about them)
ifeq ($(strip $(TEST_DISCOVER)),1)
   ifndef LITMUS_FILES
   ifndef no_collect_tests
      # if fail, ignore
      out := $(shell ($(MAKE_TEST_LIST_CMD) $(quiet) $(LITMUS_TESTS) 2>/dev/null) || echo "FAIL")

      # if the script that generates groups.c fails
      # then if there is no groups.c copy one from the backup
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
litmus_BIN_FILES := $(addprefix bin/,$(LITMUS_FILES:.c=.o))

# determine if we should run the linter
# or not
ifeq ($(quiet),0)
ifeq ($(NO_LINT),0)
  litmus_run_linter = 1
endif
endif

# running the linter on all the files at the same time
# so that we cut down on the startup time of the linter script
# but also it allows the linter to check for consistency between tests
# e.g. uniqueness of C idents
ifdef litmus_run_linter
bin/litmus/%.o: \
   __linter_success := \
      $(if $(shell $(LINTER) $(LITMUS_TEST_FILES) &> litmus/linter.log || echo FAIL), \
           $(warning $$(LINTER) invokation failed) \
		   $(warning Skipping LINT step for remaining litmus files...) \
		   $(if $(filter 1,$(firstword $(DEBUG))),\
		     $(warning see error (base64): '$(shell cat litmus/linter.log | base64)'), \
		   ) \
       )
endif


bin/litmus/%.o: litmus/%.c | check_cross_tools_exist
	$(run_cc)
ifdef litmus_run_linter
# grep through pre-computed lints
	@grep -e '$^' litmus/linter.log || true
endif

# Disable debug object symbols for litmus tests themselves
# this just makes everything so big and slow ...
bin/litmus/litmus_tests/%.o: DEBUG_CFLAGS=

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

bin/litmus.bin: bin/litmus.elf
	$(call run_cmd,OBJCOPY,$@,\
		$(OBJCOPY) -O binary bin/litmus.elf bin/litmus.bin)

# re-run the linter but with verbose output
# technically this means if we run `make build lint` we run the linter twice
# but that's alright I guess
lint:
	@$(LINTER) $(LITMUS_TEST_FILES) --verbose

collect-litmus:
	$(call run_cmd,make,Collecting new tests,\
		 $(MAKE_TEST_LIST_CMD) $(quiet) $(LITMUS_TESTS) || { \
			echo "Warning: failed to update groups.c. test listing may be out-of-sync." 1>&2 ; \
			( which python3 &> /dev/null ) && echo "LITMUS_TESTS target requires python3." 1>&2 ; \
		};\
	)
	@echo 're-run `make build` to compile any new or updated litmus tests'

debug-litmus: qemu_litmus
	{ QEMU_ARGS='-s -S' ./bin/qemu_litmus.exe $(BIN_ARGS) & echo $$! > bin/.debug.pid; }
	echo PID=`cat bin/.debug.pid`
	$(GDB) --eval-command "target remote localhost:1234"
	{ cat bin/.debug.pid | xargs kill -9 ; rm bin/.debug.pid; }

count-litmus-tests:
	@echo Found `cat litmus/test_list.txt | wc -l` tests
	@echo With `cat litmus/test_list.txt | awk '{print gensub(/^(.+\/)+(.+)\+(.+)$$/,"\\\\2","g",$$2)}' | sort | uniq | wc -l` unique shapes