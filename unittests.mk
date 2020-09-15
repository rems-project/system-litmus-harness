# list of tests
TESTS = .

UNITTESTS_FILES := $(wildcard unittests/*.c) $(wildcard unittests/testlib/*.c) $(wildcard unittests/tests/**/*.c)
unittests_BIN_FILES := $(addprefix bin/,$(UNITTESTS_FILES:.c=.o))

bin/unittests/%.o: OTHER_INCLUDES=-I unittests/include
bin/unittests/%.o: unittests/%.c | check_cross_tools_exist
	$(run_cc)

# this is ugly
.PHONY: unittests/main.c
bin/unittests/main.o: CFLAGS+=-DNO_TEST_FILES=$(strip $(shell ./unittests/getunittests.sh $(TESTS)))

bin/unittests.elf: $(COMMON_BIN_FILES) $(unittests_BIN_FILES)
	$(call run_cmd,LD,$@,\
		$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(unittests_BIN_FILES))
	$(call run_cmd,OBJDUMP,$@.S,\
		$(OBJDUMP) $(OBJDUMPFLAGS) -D $@ > $@.S)

bin/unittests.bin: bin/unittests.elf
	$(call run_cmd,OBJCOPY,$@,\
		$(OBJCOPY) -O binary bin/unittests.elf bin/unittests.bin)