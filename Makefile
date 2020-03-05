CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
QEMU = qemu-system-aarch64
OBJCOPY = aarch64-linux-gnu-objcopy
OBJDUMP = aarch64-linux-gnu-objdump
OUT_NAME = bin/main.bin
SSH_NAME = pi@rems-rpi4b
RUN_CMD_HOST = 	\
	$(QEMU) \
		-nodefaults -machine virt,accel=kvm,gic-version=host -cpu host \
		-device virtio-serial-device -device virtconsole,chardev=ctd \
		-chardev testdev,id=ctd -device pci-testdev -display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 # -initrd /tmp/tmp.UUenc9WRhz
RUN_CMD_LOCAL = 	\
	$(QEMU) \
		-nodefaults -machine virt -cpu cortex-a57 \
		-device virtio-serial-device -device virtconsole,chardev=ctd \
		-chardev testdev,id=ctd -device pci-testdev -display none -serial stdio \
		-kernel $(OUT_NAME) -smp 4 # -initrd /tmp/tmp.UUenc9WRhz

CCERRORS = return-type parentheses
CCNOWARN = unused-variable
CFLAGS = -O0 -nostdlib -I inc/ -I inc/litmus -I inc/vmm -ffreestanding -fomit-frame-pointer -fno-pie -fno-pic -Wall $(addprefix -Wno-,$(CCNOWARN)) $(addprefix -Werror=,$(CCERRORS)) -DTRACE
LDFLAGS = -nostdlib -n -pie
SSHFLAGS = -K

LIB_FILES = $(wildcard lib/*.c) $(wildcard lib/vmm/*.c)
LITMUS_FILES = $(wildcard litmus/*.c)

TOP_ASM_FILES = $(wildcard *.S)
TOP_C_FILES = $(wildcard *.c)

ASM_FILES = $(TOP_ASM_FILES)
C_FILES = $(LIB_FILES) $(LITMUS_FILES) $(TOP_C_FILES)
BIN_FILES = $(addprefix bin/,$(C_FILES:.c=.o) $(ASM_FILES:.S=.o))


.PHONY: all
all: bin/main.bin bin/litmus.exe

bindir:
	mkdir -p bin/lib/
	mkdir -p bin/litmus/
	mkdir -p bin/lib/vmm/

bin/lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

bin/litmus/%.o: litmus/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

bin/cpu_entry.o:  cpu_entry.S
	$(CC) $(CFLAGS) -c -o bin/cpu_entry.o cpu_entry.S

bin/vector_table.o:  vector_table.S
	$(CC) $(CFLAGS) -c -o bin/vector_table.o vector_table.S

bin/main.o: main.c
	$(CC) $(CFLAGS) -c -o bin/main.o main.c

bin/main.elf: $(BIN_FILES)
	$(LD) $(LDFLAGS) -o bin/main.elf -T bin.lds $(BIN_FILES)
	$(OBJDUMP) -D -r bin/main.elf > bin/main.elf.S

bin/main.bin: bindir bin/main.elf
	$(OBJCOPY) -O binary bin/main.elf bin/main.bin

run: bin/main.bin
	$(RUN_CMD_LOCAL)

debug: bin/main.bin
	{ $(RUN_CMD_LOCAL) -s -S & echo $$! > bin/.debug.pid; }
	gdb-multiarch  --eval-command "set arch aarch64" --eval-command "target remote localhost:1234"
	{ cat bin/.debug.pid | xargs kill $$pid ; rm bin/.debug.pid; }

bin/litmus.exe: OUT_NAME=$$tmp
bin/litmus.exe: bin/main.bin
	echo 'echo Starting litmus.exe' > bin/litmus.exe
	echo 'echo tmp=`mktemp`' >> bin/litmus.exe 
	echo 'tmp=`mktemp`' >> bin/litmus.exe
	echo 'echo base64 -d \<\< BIN_EOF \| zcat \> $$tmp \|\| exit 2' >> bin/litmus.exe
	echo 'base64 -d << BIN_EOF | zcat > $$tmp || exit 2' >> bin/litmus.exe
	gzip -c bin/main.bin | base64 >> bin/litmus.exe
	echo "BIN_EOF" >> bin/litmus.exe
	echo '$(RUN_CMD_HOST)' >> bin/litmus.exe
	chmod +x bin/litmus.exe

ssh: bin/litmus.exe
	scp bin/litmus.exe $(SSH_NAME):litmus.exe
	ssh $(SSHFLAGS) $(SSH_NAME) "./litmus.exe"

.PHONY: clean
clean:
	rm -rf bin/
