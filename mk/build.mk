OUT_NAME = bin/litmus.bin

TOP_ASM_FILES := $(wildcard *.S)
TOP_C_FILES := $(wildcard *.c)

LIB_FILES := $(foreach DIR,$(LIB_DIRS), $(wildcard $(DIR)/*.c))
ASM_FILES := $(TOP_ASM_FILES)
C_FILES := $(LIB_FILES) $(TOP_C_FILES)
COMMON_BIN_FILES := $(addprefix bin/,$(C_FILES:.c=.o) $(ASM_FILES:.S=.o))

RUN_CMD =

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

# checks that the specified cross-compilation tools actually exist on the machine
.PHONY: check_cross_tools_exist
check_cross_tools_exist:
ifeq ($(NO_CHECK),0)
	$(call check_tool,CC,$(CC))
	$(call check_tool,LD,$(LD))
	$(call check_tool,OBJCOPY,$(OBJCOPY))
# we don't check for Linter here
# instead we check on invokation of litmus.mk and selectively disable it
# if we don't find it
endif

# Builds a standalone executable from the elf/bin file
# Usage: $(call make_exe,QEMU-COMMAND)
define make_exe
	{	echo 'tmp=`mktemp`' > $@ ; \
		echo 'base64 -d << BIN_EOF | zcat > $$tmp || exit 2' >> $@ ; \
		gzip -c $< | base64 >> $@ ; \
		echo "BIN_EOF" >> $@ ; \
		echo '$1' >> $@ ; \
		chmod +x $@ ; \
	}
endef

# Targets for common files

bin/lib/%.o: lib/%.c | check_cross_tools_exist
	$(run_cc)

bin/cpu_entry.o:  cpu_entry.S
	$(run_cc)

bin/vector_table.o:  vector_table.S
	$(run_cc)

bin/asm_debug.o:  asm_debug.S
	$(run_cc)

# generic executable file builder

bin/debug_%.exe: OUT_NAME=$$tmp
bin/debug_%.exe: bin/%.bin
	$(call run_cmd,BUILD_EXE,$@,\
		$(call make_exe,$(RUN_CMD_LOCAL) -S -s)\
	)