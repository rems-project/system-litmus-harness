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
		-kernel ./main.bin -smp 1 # -initrd /tmp/tmp.UUenc9WRhz


LIB = inc/
LIB_FILES = lib/asm_wrappers.c lib/printer.c lib/abort.c lib/psci.c
CFLAGS = -O0 -I $(LIB)
LDFLAGS = -nostdlib

.PHONY: all
all: main.bin

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $^

cpu_entry.o:  cpu_entry.S
	$(CC) $(CFLAGS) -c -nostdlib -o cpu_entry.o cpu_entry.S

main.o: main.c
	$(CC) $(CFLAGS) -c -nostdlib -o main.o main.c

main.elf: cpu_entry.o main.o $(LIB_FILES:.c=.o)
	$(LD) $(LDFLAGS) -o main.elf -T bin.lds main.o cpu_entry.o $(LIB_FILES:.c=.o)
	$(OBJDUMP) -D main.elf > main.elf.S


main.bin: main.elf
	$(OBJCOPY) -O binary $^ $@

run:
	$(RUN_CMD)

debug:
	{ $(RUN_CMD) -s -S & echo $$! > .debug.pid; }
	gdb-multiarch  --eval-command "set arch aarch64" --eval-command "target remote localhost:1234"
	{ cat .debug.pid | xargs kill $$pid && rm .debug.pid; }


.PHONY: clean
clean:
	rm -f *.o
	rm -f lib/*.o
	rm -f *.elf*
	rm -f *.bin*
