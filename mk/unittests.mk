.PHONY: run-unittests
.PHONY: build-unittests
.PHONY: collect-unittests

# list of tests
TESTS = .

UNITTESTS_FILES := $(wildcard unittests/*.c) $(wildcard unittests/testlib/*.c) $(wildcard unittests/tests/**/*.c)
unittests_BIN_FILES := $(addprefix bin/,$(UNITTESTS_FILES:.c=.o))

collect-unittests:
	@{ out=`$(SH) ./unittests/getunittests.sh $(TESTS)` ; \
	   numTests=`echo $$out | awk '{print $$1}'` ; \
	   numFiles=`echo $$out | awk '{print $$2}'` ; \
	   echo "Found:" ; \
	   cat ./unittests/tests.list ; \
	   echo "Collected $${numTests} unit tests over $${numFiles} files" ; }
	@echo 're-run `make unittests` to compile and run any new or updated unit tests'

debug-unittests: bin/debug_unittests.exe
	{ ./bin/debug_unittests.exe $(BIN_ARGS) & echo $$! > bin/.debug.pid; }
	$(GDB) --eval-command "target remote localhost:1234"
	{ cat bin/.debug.pid | xargs kill $$pid ; rm bin/.debug.pid; }


bin/unittests/%.o: OTHER_INCLUDES=-I unittests/include
bin/unittests/%.o: unittests/%.c | check_cross_tools_exist
	$(run_cc)

# this is ugly
.PHONY: unittests/main.c
bin/unittests/main.o: CFLAGS+=-DNO_TEST_FILES="$(strip $(shell ./unittests/getunittests.sh $(TESTS) | awk '{print $$2}'))"

bin/unittests.elf: $(COMMON_BIN_FILES) $(unittests_BIN_FILES)
	$(call run_cmd,LD,$@,\
		$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(unittests_BIN_FILES))
	$(call run_cmd,OBJDUMP,$@.S,\
		$(OBJDUMP) $(OBJDUMPFLAGS) -D $@ > $@.S)

bin/unittests.bin: bin/unittests.elf
	$(call run_cmd,OBJCOPY,$@,\
		$(OBJCOPY) -O binary bin/unittests.elf bin/unittests.bin)

bin/qemu_unittests.exe: OUT_NAME=bin/unittests.bin

# top-level targets
qemu_unittests.exe: bin/qemu_unittests.exe
	$(call run_cmd,INSTALL,./$@, \
		@cp bin/qemu_unittests.exe qemu_unittests)

kvm_unittests.exe: bin/kvm_unittests.exe
	$(call run_cmd,INSTALL,./$@, \
		@cp bin/kvm_unittests.exe kvm_unittests)

build-unittests: qemu_unittests.exe kvm_unittests.exe

run-unittests: bin/qemu_unittests.exe
	./bin/qemu_unittests.exe $(BIN_ARGS)