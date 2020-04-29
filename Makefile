KNOWN_PREFIXES = aarch64-linux-gnu- aarch64-none-elf-
PREFIX = $(word 1, $(foreach PRF,$(KNOWN_PREFIXES),$(if $(shell which $(PRF)gcc),$(PRF),)))

CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
QEMU = qemu-system-aarch64
OUT_NAME = bin/litmus.bin
SSH_NAME = pi@rems-rpi4b
BIN_ARGS =
TESTS = .
RUN_CMD_HOST = 	\
	$(QEMU) \
		-nodefaults -machine virt,accel=kvm,gic-version=host -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"
RUN_CMD_LOCAL = 	\
	$(QEMU) \
		-nodefaults -machine virt -cpu cortex-a57 \
		-device virtio-serial-device -device virtconsole,chardev=ctd \
		-chardev testdev,id=ctd -device pci-testdev -display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

CCERRORS = return-type parentheses misleading-indentation null-dereference sequence-point uninitialized maybe-uninitialized
CCNOWARN =
INC_DIRS = inc/ inc/litmus/ inc/vmm/ inc/re/
LIB_DIRS = lib/ lib/arch/ lib/vmm/ lib/valloc/ lib/re/
OTHER_INCLUDES =  # set for unittests
CFLAGS = -O0 -nostdlib \
		$(foreach DIR,$(INC_DIRS),-I $(DIR)) \
		$(foreach DIR,$(OTHER_INCLUDES),-I $(DIR)) \
		-ffreestanding -fomit-frame-pointer -fno-pie -fno-pic \
		-Wall $(addprefix -Wno-,$(CCNOWARN)) $(addprefix -Werror=,$(CCERRORS)) \

LDFLAGS = -nostdlib -n -pie
SSHFLAGS =

LIB_FILES = $(foreach DIR,$(LIB_DIRS), $(wildcard $(DIR)/*.c))
LITMUS_FILES = $(wildcard litmus/*.c) $(wildcard litmus/litmus_tests/**/*.c)
UNITTESTS_FILES = $(wildcard unittests/*.c) $(wildcard unittests/testlib/*.c) $(wildcard unittests/tests/**/*.c)

TOP_ASM_FILES = $(wildcard *.S)
TOP_C_FILES = $(wildcard *.c)

ASM_FILES = $(TOP_ASM_FILES)
C_FILES = $(LIB_FILES) $(TOP_C_FILES)
COMMON_BIN_FILES = $(addprefix bin/,$(C_FILES:.c=.o) $(ASM_FILES:.S=.o))
litmus_BIN_FILES = $(addprefix bin/,$(LITMUS_FILES:.c=.o))
unittests_BIN_FILES = $(addprefix bin/,$(UNITTESTS_FILES:.c=.o))

.PHONY: all
all: bin/litmus.exe bin/unittests.exe bin/qemu_litmus.exe bin/qemu_unittests.exe

bindir:
	mkdir -p $(dir $(COMMON_BIN_FILES))
	mkdir -p $(dir $(unittests_BIN_FILES))
	mkdir -p $(dir $(litmus_BIN_FILES))

bin/lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: unittests/main.c
bin/unittests/main.o: CFLAGS+=-DNO_TEST_FILES=$(strip $(shell ./unittests/getunittests.sh $(TESTS)))

bin/unittests/%.o: OTHER_INCLUDES=-I unittests/include
bin/unittests/%.o: unittests/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

bin/litmus/%.o: litmus/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

bin/cpu_entry.o:  cpu_entry.S
	$(CC) $(CFLAGS) -c -o bin/cpu_entry.o cpu_entry.S

bin/vector_table.o:  vector_table.S
	$(CC) $(CFLAGS) -c -o bin/vector_table.o vector_table.S

bin/litmus.o: litmus_tests/main.c
	$(CC) $(CFLAGS) -c -o bin/litmus.o litmus_tests/main.c

bin/litmus.elf: $(COMMON_BIN_FILES) $(litmus_BIN_FILES)
	$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(litmus_BIN_FILES)
	$(OBJDUMP) -D -r $@ > $@.S

bin/unittests.elf: $(COMMON_BIN_FILES) $(unittests_BIN_FILES)
	$(LD) $(LDFLAGS) -o $@ -T bin.lds $(COMMON_BIN_FILES) $(unittests_BIN_FILES)
	$(OBJDUMP) -D -r $@ > $@.S

bin/litmus.bin: bindir bin/litmus.elf
	$(OBJCOPY) -O binary bin/litmus.elf bin/litmus.bin

bin/unittests.bin: bindir bin/unittests.elf
	$(OBJCOPY) -O binary bin/unittests.elf bin/unittests.bin

debug: bin/litmus.bin
	{ $(RUN_CMD_LOCAL) -s -S & echo $$! > bin/.debug.pid; }
	gdb-multiarch  --eval-command "set arch aarch64" --eval-command "target remote localhost:1234"
	{ cat bin/.debug.pid | xargs kill $$pid ; rm bin/.debug.pid; }

bin/%.exe: OUT_NAME=$$tmp
bin/%.exe: bin/%.bin
	echo 'set -o xtrace' > $@
	echo 'echo Starting $@' >> $@
	echo 'tmp=`mktemp`' >> $@
	echo 'base64 -d << BIN_EOF | zcat > $$tmp || exit 2' >> $@
	gzip -c $^ | base64 >> $@
	echo "BIN_EOF" >> $@
	echo '$(RUN_CMD_HOST)' >> $@
	chmod +x $@

bin/qemu_%.exe: OUT_NAME=$$tmp
bin/qemu_%.exe: bin/%.bin
	echo 'set -o xtrace' > $@
	echo 'echo Starting $@' >> $@
	echo 'tmp=`mktemp`' >> $@
	echo 'base64 -d << BIN_EOF | zcat > $$tmp || exit 2' >> $@
	gzip -c $^ | base64 >> $@
	echo "BIN_EOF" >> $@
	echo '$(RUN_CMD_LOCAL)' >> $@
	chmod +x $@

run: bin/qemu_litmus.exe
	./bin/qemu_litmus.exe $(BIN_ARGS)

unittests: OUT_NAME=bin/unittests.bin
unittests: bin/unittests.bin
	./bin/qemu_unittests.exe

ssh: bin/litmus.exe
	scp bin/litmus.exe $(SSH_NAME):litmus.exe
	ssh $(SSHFLAGS) $(SSH_NAME) "./litmus.exe '$(BIN_ARGS)'"

.PHONY: clean
clean:
	rm -rf bin/
