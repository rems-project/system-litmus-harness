CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
QEMU = qemu-system-aarch64
OBJCOPY = aarch64-linux-gnu-objcopy
OBJDUMP = aarch64-linux-gnu-objdump
RUN_CMD = 	\
	$(QEMU) \
		-nodefaults -machine virt,accel=tcg -cpu cortex-a57 \
		-device virtio-serial-device -device virtconsole,chardev=ctd \
		-chardev testdev,id=ctd -device pci-testdev -display none -serial stdio \
		-kernel ./main.bin -smp 4 # -initrd /tmp/tmp.UUenc9WRhz


LIB_FILES = $(wildcard lib/*.c)
LITMUS_FILES = $(wildcard litmus/*.c)
CFLAGS = -O0 -I inc/ -I inc/litmus -ffreestanding -fomit-frame-pointer -fno-pie -fno-pic # -DTRACE
LDFLAGS = -nostdlib -n -pie

.PHONY: all
all: main.bin

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $^

litmus/%.o: litmus/%.c
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $^

cpu_entry.o:  cpu_entry.s
	$(CC) $(CFLAGS) -c -nostdlib -o cpu_entry.o cpu_entry.s

vector_table.o:  vector_table.s
	$(CC) $(CFLAGS) -c -nostdlib -o vector_table.o vector_table.s

main.o: main.c
	$(CC) $(CFLAGS) -c -nostdlib -o main.o main.c

main.elf: vector_table.o cpu_entry.o main.o $(LIB_FILES:.c=.o) $(LITMUS_FILES:.c=.o)
	$(LD) $(LDFLAGS) -o main.elf -T bin.lds main.o cpu_entry.o vector_table.o $(LIB_FILES:.c=.o) $(LITMUS_FILES:.c=.o)
	$(OBJDUMP) -D -r main.elf > main.elf.s


main.bin: main.elf
	$(OBJCOPY) -O binary $^ $@

run:
	$(RUN_CMD)

debug:
	{ $(RUN_CMD) -s -S & echo $$! > .debug.pid; }
	gdb-multiarch  --eval-command "set arch aarch64" --eval-command "target remote localhost:1234"
	{ cat .debug.pid | xargs kill $$pid ; rm .debug.pid; }


.PHONY: clean
clean:
	rm -f *.o
	rm -f lib/*.o
	rm -f *.elf*
	rm -f *.bin*
	rm -f .debug.pid
